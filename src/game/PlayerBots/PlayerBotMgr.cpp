#include "Common.h"
#include <time.h>
#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <string>
#include "Policies/SingletonImp.h"
#include "PlayerBotMgr.h"
#include "Logging.h"
#include "ObjectMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "AccountMgr.h"
#include "Auth/BigNumber.h"
#include "Opcodes.h"
#include "Config/Config.h"
#include "Chat.h"
#include "Player.h"
#include "PlayerBotAI.h"
#include "Bot/PlayerbotFactory.h"
#include "Util/ServerFacade.h"
#include "Util/PlayerTravelMgr.h"
#include "Util/PlayerQuestMgr.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "Bot/PlayerbotAIBase.h"
#include "Util/PlayerbotAIConfig.h"
#include "Util/PerfMonitor.h"
#include "Agent/BotStateSnapshot.h"
#include "Agent/BotCommandAPI.h"
#include "Anticheat.h"
#include "Log.h"
#include "Logging.h"
#include "Item.h"
#include "ItemPrototype.h"
#include "Mgr/Item/StatsWeightCalculator.h"
#include <set>

// ---------------------------------------------------------------------------
// R3a P1: one-time item score dump (PlayerBot.DebugScoreDump)
//
// Verifies the StatsWeightCalculator pipeline end-to-end on live data:
// base item stats, item-spell auras (flat % hit/crit, MOD_STAT index),
// and green suffixes (chance-weighted class average; instance-exact when
// the suffix is already rolled). Dumps the first 5 bots: up to 8 bag
// items each (instance scoring) + 3 reference items (proto scoring):
//   727    Notched Shortsword (ilvl 10, green, suffix class 5168)
//   10030  Admiral's Hat      (ilvl 48, broken effect-35 spell — NEGATIVE
//           control: should score floor-only, the engine applies nothing)
//   20130  Diamond Flask      (ilvl 52, on-use +49 STR — 0.15x multiplier)
//   3341   Gauntlets of Ogre Strength (ilvl 32 q2, +16 AP equip spell)
//   3841   Golden Scale Shoulders     (ilvl 35 q3, +1% haste equip spell)
// ---------------------------------------------------------------------------
namespace
{
    std::set<std::string> s_scoreDumpedBots;

    void DumpItemScores(Player* bot)
    {
        StatsWeightCalculator calculator(bot);
        char const* name = bot->GetName();

        sLog.outInfo("playerbots: scores: %s (lvl %u class %u) collector=%u",
            name, bot->GetLevel(), bot->GetClass(), uint32(calculator.GetCollectorType()));

        // Bag items (instance-accurate suffix scoring), max 8
        uint32 dumped = 0;
        for (uint8 bag = 0; bag < INVENTORY_SLOT_BAG_END && dumped < 8; ++bag)
        {
            for (uint8 slot = 0; slot < MAX_BAG_SIZE && dumped < 8; ++slot)
            {
                Item* item = bot->GetItemByPos(bag, slot);
                if (!item)
                    continue;
                ItemPrototype const* proto = item->GetProto();
                sLog.outInfo("playerbots: scores: %s bag %u '%s' (ilvl %u q%u) suffix=%d score=%.1f",
                    name, proto->ItemId, proto->Name1.c_str(), proto->ItemLevel, proto->Quality,
                    item->GetItemRandomPropertyId(), calculator.CalculateItem(item));
                ++dumped;
            }
        }

        // Reference items (proto-level, class-average suffixes)
        static const uint32 refItems[] = { 727, 10030, 20130, 3341, 3841 };
        for (size_t i = 0; i < sizeof(refItems) / sizeof(refItems[0]); ++i)
        {
            ItemPrototype const* proto = sObjectMgr.GetItemPrototype(refItems[i]);
            if (!proto)
                continue;
            sLog.outInfo("playerbots: scores: %s ref %u '%s' (ilvl %u q%u) score=%.1f",
                name, proto->ItemId, proto->Name1.c_str(), proto->ItemLevel, proto->Quality,
                calculator.CalculateItem(proto->ItemId));
        }
    }
}

// Forward declaration (defined in CharacterHandler.cpp)
class LoginQueryHolder;
void ScheduleBotLogin(uint32 accountId, ObjectGuid playerGuid);

PlayerBotMgr sPlayerBotMgr;

PlayerBotMgr::PlayerBotMgr()
{
    totalChance = 0;
    _maxAccountId = 0;

    /* Config */
    confMinBots = 4;
    confMaxBots = 8;
    confBotsRefresh = 30000;
    confUpdateDiff = 10000;
    enable = false;
    confDebug = false;
    forceLogoutDelay = true;
    confAsyncLogin = true;
    confAsyncBatchSize = 5;

    /* Time */
    m_elapsedTime = 0;
    m_lastBotsRefresh = 0;
    m_lastUpdate = 0;
    m_lastStatsPrint = 0;
}

PlayerBotMgr::~PlayerBotMgr()
{

}

void PlayerBotMgr::LoadConfig()
{
    enable = sConfig.GetBoolDefault("PlayerBot.Enable", false);
    confMinBots = sConfig.GetIntDefault("PlayerBot.MinBots", 3);
    confMaxBots = sConfig.GetIntDefault("PlayerBot.MaxBots", 10);
    confBotsRefresh = sConfig.GetIntDefault("PlayerBot.Refresh", 60000);
    confDebug = sConfig.GetBoolDefault("PlayerBot.Debug", false);
    confUpdateDiff = sConfig.GetIntDefault("PlayerBot.UpdateMs", 10000);
    forceLogoutDelay = sConfig.GetBoolDefault("PlayerBot.ForceLogoutDelay", true);
    if (!forceLogoutDelay)
        m_tempBots.clear();

    confFactoryEnabled = sConfig.GetBoolDefault("PlayerBot.FactoryEnabled", true);
    confFactoryBotCount = sConfig.GetIntDefault("PlayerBot.FactoryBotCount", 10);
    confFactoryAccountPrefix = sConfig.GetStringDefault("PlayerBot.FactoryAccountPrefix", "botacc");

    // AC pattern: async login config
    confAsyncLogin = sConfig.GetBoolDefault("PlayerBot.AsyncLogin", true);
    confAsyncBatchSize = sConfig.GetIntDefault("PlayerBot.AsyncLoginBatchSize", 5);
}

void PlayerBotMgr::Load()
{
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerBotMgr::Load() START");
    // 1- clean
    DeleteAll();
    m_bots.clear();
    m_tempBots.clear();
    m_loginQueue.clear();
    m_loadingBots.clear();
    totalChance = 0;

    // 2- Configuration
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerBotMgr::Load() calling LoadConfig()");
    LoadConfig();
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerBotMgr::Load() LoadConfig() done, factory=%d, count=%u, async=%d", confFactoryEnabled, confFactoryBotCount, confAsyncLogin);

    // 2.5- AC pattern: DeleteAllBots mode (AiPlayerbot.DeleteRandomBotAccounts)
    // When set to 1, delete all bot accounts/characters and shutdown
    if (sPlayerbotAIConfig.deleteAllBots)
    {
        LOG_INFO("playerbots", "DeleteAllBots mode enabled - deleting all bot accounts and characters...");
        PlayerbotFactory::DeleteAllBots(confFactoryAccountPrefix);
        LOG_INFO("playerbots", "Bot cleanup complete. Please set PlayerBot.DeleteAllBots=0 and restart.");
        // Schedule graceful shutdown (calling StopNow during init crashes socket manager)
        sWorld.ShutdownServ(1, SHUTDOWN_MASK_RESTART, SHUTDOWN_EXIT_CODE);
        return;
    }

    // 2.6- R7: quest pipeline startup caches (takers / objective POIs /
    // giver entries — one-time SQL, same pattern as the travel hub cache).
    // L3: unconditional — the cache is the quest DATA layer; the quest ACTIONS
    // (sweep/accept) stay gated by questEnabled. Load() is a no-op re-run guard.
    PlayerQuestMgr::Load();

    // 3- Load usable account ID
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerBotMgr::Load() querying MAX(id)");
    QueryResult *result = LoginDatabase.PQuery("SELECT MAX(id) FROM account");
    if (!result)
    {
        sLog.outError("Playerbot: unable to load max account id.");
        return;
    }
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerBotMgr::Load() querying Fetch()");
    Field *fields = result->Fetch();
    _maxAccountId = fields[0].GetUInt32() + 10000;
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerBotMgr::Load() maxAccountId=%u", _maxAccountId);
    delete result;

    // 3.5- Generate bots via factory if enabled (AC pattern: checks existing chars)
    if (confFactoryEnabled)
    {
        LOG_DEBUG("playerbots", "[3ENGINE] PlayerBotMgr::Load() calling GenerateBots(%u, '%s')", confFactoryBotCount, confFactoryAccountPrefix.c_str());
        PlayerbotFactory::GenerateBots(confFactoryBotCount, confFactoryAccountPrefix);
        LOG_DEBUG("playerbots", "[3ENGINE] PlayerBotMgr::Load() GenerateBots() done");
    }

    // 4- LoadFromDB
    result = CharacterDatabase.PQuery("SELECT char_guid, chance, ai FROM playerbot");
    if (!result)
        LOG_DEBUG("playerbots", "Loading playerbots...");
    else
    {
        do
        {
            fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            uint32 acc = sObjectMgr.GetPlayerAccountIdByGUID(guid);
            uint32 chance = fields[1].GetUInt32();

            PlayerBotEntry* entry = new PlayerBotEntry(guid, acc, chance);
            entry->ai = CreatePlayerBotAI(fields[2].GetCppString());
            entry->ai->botEntry = entry;
            if (!sObjectMgr.GetPlayerNameByGUID(guid, entry->name))
                entry->name = "<Unknown>";
            entry->ai->OnBotEntryLoad(entry);
            m_bots[entry->playerGUID] = entry;
            totalChance += chance;
        } while (result->NextRow());

        delete result;
        LOG_DEBUG("playerbots", "%u bots loaded from playerbot table", (uint32)m_bots.size());
    }

    // 5- Check config/DB
    if (confMinBots > m_bots.size() && !m_bots.empty())
        confMinBots = m_bots.size();
    if (confMaxBots > m_bots.size())
        confMaxBots = m_bots.size();
    if (confMaxBots <= confMinBots)
        confMaxBots = confMinBots + 1;

    // 6- Start initial bots
    if (enable)
    {
        if (confAsyncLogin)
        {
            // AC pattern: queue all initial bot logins
            uint32 queued = 0;
            for (uint32 i = 0; i < confMinBots && !m_bots.empty(); ++i)
            {
                // Find an offline bot
                for (std::map<uint32, PlayerBotEntry*>::iterator it = m_bots.begin(); it != m_bots.end(); ++it)
                {
                    if (it->second->state == PB_STATE_OFFLINE && !it->second->customBot)
                    {
                        AddBotAsync((uint32)it->first);
                        ++queued;
                        break;
                    }
                }
            }
            LOG_DEBUG("playerbots", "[PlayerBotMgr] Queued %u bots for async login", queued);
        }
        else
        {
            // Legacy: sync login
            for (uint32 i = 0; i < confMinBots; i++)
                AddRandomBot();
        }
    }

    //7 - Remplir les stats
    m_stats.confMaxOnline = confMaxBots;
    m_stats.confMinOnline = confMinBots;
    m_stats.totalBots = m_bots.size();
    m_stats.confBotsRefresh = confBotsRefresh;
    m_stats.confUpdateDiff = confUpdateDiff;

    //8- Afficher les stats si débug
    if (confDebug)
    {
        LOG_DEBUG("playerbots", "[PlayerBotMgr] Between %u and %u bots online", confMinBots, confMaxBots);
        LOG_DEBUG("playerbots", "[PlayerBotMgr] %u now loading", m_stats.loadingCount);
        LOG_DEBUG("playerbots", "[PlayerBotMgr] Async login: %s (batch=%u)", confAsyncLogin ? "ON" : "OFF", confAsyncBatchSize);
    }
}

void PlayerBotMgr::DeleteAll()
{
    m_stats.onlineCount = 0;
    m_stats.loadingCount = 0;

    std::map<uint32, PlayerBotEntry*>::iterator i;
    for (i = m_bots.begin(); i != m_bots.end(); i++)
    {
        if (i->second->state != PB_STATE_OFFLINE)
        {
            OnBotLogout(i->second);
            totalChance += i->second->chance;
        }
    }

    m_tempBots.clear();
    m_loginQueue.clear();
    m_loadingBots.clear();

    if (confDebug)
        LOG_DEBUG("playerbots", "[PlayerBotMgr] Deleting all bots [OK]");
}

void PlayerBotMgr::OnBotLogin(PlayerBotEntry *e)
{
    e->state = PB_STATE_ONLINE;
    m_loadingBots.erase((uint32)e->playerGUID);
    if (confDebug)
        LOG_DEBUG("playerbots", "[PlayerBot][Login]  '%s' GUID:%u Acc:%u", e->name.c_str(), e->playerGUID, e->accountId);
}

void PlayerBotMgr::OnBotLogout(PlayerBotEntry *e)
{
    e->state = PB_STATE_OFFLINE;
    m_loadingBots.erase((uint32)e->playerGUID);
    if (confDebug)
        LOG_DEBUG("playerbots", "[PlayerBot][Logout] '%s' GUID:%u Acc:%u", e->name.c_str(), e->playerGUID, e->accountId);
}

void PlayerBotMgr::OnPlayerInWorld(Player* player)
{
    if (PlayerBotEntry* e = player->GetSession()->GetBot())
    {
        LOG_DEBUG("playerbots", "[OnPlayerInWorld] Setting AI for bot '%s'", player->GetName());
        player->setAI(e->ai);
        e->ai->SetPlayer(player);
        e->ai->OnPlayerLogin();
        LOG_DEBUG("playerbots", "[OnPlayerInWorld] AI initialized for bot '%s'", player->GetName());

        // R3a P1: one-time score dump (first 5 bots) — PlayerBot.DebugScoreDump
        if (sPlayerbotAIConfig.debugScoreDump && s_scoreDumpedBots.size() < 5 &&
            s_scoreDumpedBots.insert(player->GetName()).second)
            DumpItemScores(player);
    }
    else
    {
        LOG_DEBUG("playerbots", "[OnPlayerInWorld] No bot entry for player '%s'", player->GetName());
    }
}

// R5e.2: emergency relocation — teleport a bot to a level/faction-matched
// TOWN (the same destination pool as scheduled travel: 25% city / 50% quest
// POI / rest inn-flight-bank hubs). R5e.2 retired the old mob-anchored
// relocation (land next to a hostile mob's spawn) after the night-verify: a
// lvl-12 bot was dropped into a Venture Co lvl-14-17 camp and ping-ponged.
// The town pool is level-matched (±5/±2 brackets) so the ambient mobs around
// the landing are grindable. Last-resort fallback: band anchor + ±150 +
// real ground height. Clears the stale "current target" via the shared
// context (NEVER Engine::Reset — it deletes strategies/triggers) and resets
// the relocation grace clock at the new spot.
static bool RelocateBotToGrindSpot(Player* bot, PlayerBotEntry* e, const char* why, bool allowCity = true)
{
    PlayerTravelMgr::TravelDest dest;
    if (PlayerTravelMgr::PickDestination(bot, dest, allowCity))
    {
        float x = dest.x + (float)((int)urand(0, 60) - 30);
        float y = dest.y + (float)((int)urand(0, 60) - 30);
        float z = dest.z;
        Map* targetMap = sMapMgr.FindMap(dest.map);
        if (targetMap)
            z = targetMap->GetHeight(x, y, dest.z);
        if (!MapManager::IsValidMapCoord(dest.map, x, y, z))
            return false; // never crash in TeleportTo (throws on bad coords)
        if (!bot->TeleportTo(dest.map, x, y, z, 0.0f))
            return false;
        if (e->ai)
            if (AiObjectContext* ctx = e->ai->GetAiObjectContext())
                ctx->GetValue<Unit*>("current target")->Set(nullptr);
        ServerFacade::MarkViableGrindTargetSeen(bot);  // fresh grace window at the new spot
        sLog.outInfo("playerbots: relocated %s bot %s (lvl %u) to %s %u (map %u %.0f,%.0f,%.0f)",
            why, bot->GetName(), bot->GetLevel(), dest.reason, dest.entry, dest.map, x, y, z);
        return true;
    }

    // No level-matched town (band gap) — last resort: band anchor + real
    // ground height (a table z is only valid at the anchor).
    PlayerbotFactory::BotSpawnPoint sp = PlayerbotFactory::PickSpawnPosition(bot->GetLevel(), bot->GetRace());
    uint32 tMap = sp.map;
    float x = sp.x + (float)irand(-150, 150);
    float y = sp.y + (float)irand(-150, 150);
    float z = sp.z;
    Map* targetMap = sMapMgr.FindMap(tMap);
    if (targetMap)
        z = targetMap->GetHeight(x, y, sp.z);
    if (!MapManager::IsValidMapCoord(tMap, x, y, z))
        return false;
    if (!bot->TeleportTo(tMap, x, y, z, 0.0f))
        return false;
    if (e->ai)
        if (AiObjectContext* ctx = e->ai->GetAiObjectContext())
            ctx->GetValue<Unit*>("current target")->Set(nullptr);
    ServerFacade::MarkViableGrindTargetSeen(bot);
    sLog.outInfo("playerbots: relocated %s bot %s (lvl %u) to band spawn (map %u %.0f,%.0f,%.0f)",
        why, bot->GetName(), bot->GetLevel(), tMap, x, y, z);
    return true;
}

// R5e stale-combat breaker clock: seconds since each bot entered its current
// combat streak. File-scope like ServerFacade's idle clock; pruned against
// m_bots so it can't outlive the population.
static std::map<uint32, time_t> s_combatSince;

void PlayerBotMgr::Update(uint32 diff)
{
    // Bots temporaires
    std::map<uint32, uint32>::iterator it;
    for (it = m_tempBots.begin(); it != m_tempBots.end(); ++it)
    {
        if (it->second < diff)
            it->second = 0;
        else
            it->second -= diff;
    }

    it = m_tempBots.begin();
    while (it != m_tempBots.end())
    {
        if (!it->second)
        {
            // Update des "chatBot" aussi.
            for (std::map<uint32, PlayerBotEntry*>::iterator iter = m_bots.begin(); iter != m_bots.end(); ++iter)
                if (iter->second->accountId == it->first)
                {
                    iter->second->state = PB_STATE_OFFLINE; // Will get logged out at next WorldSession::Update call
                    m_bots.erase(iter);
                    break;
                }
            m_tempBots.erase(it);
            it = m_tempBots.begin();
        }
        else
            ++it;
    }

    m_elapsedTime += diff;

    /* AC pattern: periodic stats output (every 30s) */
    m_lastStatsPrint += diff;
    if (m_lastStatsPrint >= 30000)
    {
        m_lastStatsPrint = 0;
        PrintStats();
    }

    /* R6.1: agent interface — compact bots.json state dump (every 30s)
       for the out-of-process agent to poll. Cheap (~30KB), gated by
       PlayerBot.AgentStateFile. */
    if (sPlayerbotAIConfig.agentStateFile)
    {
        m_lastStateDump += diff;
        if (m_lastStateDump >= 30000)
        {
            m_lastStateDump = 0;
            BotStateSnapshot::DumpStateFile();
        }
    }

    /* R6.1: agent interface — file-command queue poll (every 5s).
       Agent drops ../logs/botstate/commands/<bot>.cmd; result lands in
       ../logs/botstate/results/<bot>.res. No console involved. */
    if (sPlayerbotAIConfig.agentCmdFile)
    {
        m_lastAgentCmdPoll += diff;
        if (m_lastAgentCmdPoll >= 5000)
        {
            m_lastAgentCmdPoll = 0;
            PollAgentCommandFiles();
        }
    }

    /* R6.1 DIAG: log online-but-out-of-world bots with their teleport flags
       to pin the post-login world-drain mechanism (stuck mid-teleport?). */
    m_lastOutOfWorldDiag += diff;
    if (m_lastOutOfWorldDiag >= 60000)
    {
        m_lastOutOfWorldDiag = 0;
        int owCount = 0;
        for (auto it = GetBotsMap().begin(); it != GetBotsMap().end(); ++it)
        {
            PlayerBotEntry* e = it->second;
            if (!e || e->state != PB_STATE_ONLINE || !e->ai)
                continue;
            Player* bot = e->ai->me;
            if (!bot || bot->IsInWorld())
                continue;
            if (owCount < 25)
                sLog.outInfo("playerbots: OUT-OF-WORLD bot guid=%u name=%s teleport=%d near=%d far=%d mapId=%u pos=(%.0f,%.0f,%.0f) alive=%d",
                    bot->GetGUIDLow(), bot->GetName(), (int)bot->IsBeingTeleported(),
                    (int)bot->IsBeingTeleportedNear(), (int)bot->IsBeingTeleportedFar(),
                    (unsigned)bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), (int)bot->IsAlive());
            ++owCount;
        }
        if (owCount > 25)
            sLog.outInfo("playerbots: OUT-OF-WORLD total=%d (showing first 25)", owCount);
    }

    /* R7: quest pipeline (every 30s) — town→accept→objective-POI→turn-in
       loop (AC base quest layer + NewRPG state machine, rewritten: no
       WorldPacketTrigger in this port, so PrepareQuestMenu polling +
       proximity-gated CMSG replay). The existing grinding strategy does the
       quest kills at the POI; this mgr only decides where to be and when to
       talk. Skipped by the sweeps below while a bot chases a quest. */
    m_lastQuestSweep += diff;
    if (sPlayerbotAIConfig.questEnabled && m_lastQuestSweep >= 30000)
    {
        m_lastQuestSweep = 0;
        for (std::map<uint32, PlayerBotEntry*>::iterator i = m_bots.begin(); i != m_bots.end(); ++i)
        {
            if (i->second->state != PB_STATE_ONLINE)
                continue;
            Player* bot = ObjectAccessor::FindPlayer(i->first);
            if (!bot)
                continue;
            PlayerQuestMgr::ProcessBot(bot);
        }
    }

    // R5d idle-relocation sweep REMOVED (2026-09-20, user direction): it
    // teleported bots after N seconds idle — invented behavior AC mod-playerbots
    // does not do (AC bots WALK; teleport = stuck-recovery only, handled by the
    // L1 MoveFarTo stuck rule + the stale-combat breaker below). Bots now rely
    // on walking (R7 L2/L4/L5) + the travel system to reach viable spots.

    /* R5e: stale-combat breaker (every 30s) — bots locked in combat for
       staleCombatSeconds+ (unkillable city guards, elite camps, mob swarms
       pulled by a travel landing) never leave the COMBAT engine, and that
       blocks BOTH the idle-relocation sweep (IsInCombat guard) and travel
       (IsInCombat guard in DoTravel). Break them: drop target via the shared
       context + teleport to a grind spot. Typical kills are 30s-3min, so
       the threshold is safely above a normal fight (default 600s). */
    if (sPlayerbotAIConfig.staleCombatSeconds > 0 && m_lastCombatSweep >= 30000)
    {
        m_lastCombatSweep = 0;
        time_t now = time(nullptr);
        for (std::map<uint32, PlayerBotEntry*>::iterator i = m_bots.begin(); i != m_bots.end(); ++i)
        {
            if (i->second->state != PB_STATE_ONLINE)
                continue;
            Player* bot = ObjectAccessor::FindPlayer(i->first);
            if (!bot)
                continue;
            std::map<uint32, time_t>::iterator cs = s_combatSince.find(i->first);
            if (PlayerQuestMgr::HasActiveQuest(i->first))
                continue; // R7: let the quest abandon (5 min) beat the breaker (10 min)
            if (bot->IsInCombat() && !bot->IsBeingTeleported())
            {
                if (cs == s_combatSince.end())
                {
                    s_combatSince[i->first] = now; // combat streak starts
                    continue;
                }
                uint32 combatSecs = (uint32)(now - cs->second);
                if (combatSecs < sPlayerbotAIConfig.staleCombatSeconds)
                    continue;
                sLog.outInfo("playerbots: breaking stale combat (%us) for bot %s (lvl %u)",
                    combatSecs, bot->GetName(), bot->GetLevel());
                if (RelocateBotToGrindSpot(bot, i->second, "stale-combat", /*allowCity=*/false))
                    s_combatSince.erase(cs); // fresh start; on failure retry next sweep
            }
            else
                s_combatSince.erase(cs); // combat ended — clear the clock
        }
        for (std::map<uint32, time_t>::iterator cs = s_combatSince.begin(); cs != s_combatSince.end();)
            if (!m_bots.count(cs->first))
                cs = s_combatSince.erase(cs);
            else
                ++cs;
    }

    /* R5e: real travel (AC RandomPlayerbotMgr pattern) — per-bot random
       travel every 1-5h to a real innkeeper/flight/bank hub (25% to a
       city + homebind refresh). Runs on the 30s cadence; DoTravel
       re-checks all guards on fire and reports deferrals (retry in 30s). */
    if (sPlayerbotAIConfig.travelEnabled)
    {
        for (std::map<uint32, PlayerBotEntry*>::iterator i = m_bots.begin(); i != m_bots.end(); ++i)
        {
            if (i->second->state != PB_STATE_ONLINE)
                continue;
            Player* bot = ObjectAccessor::FindPlayer(i->first);
            if (!bot)
                continue;

            std::map<uint32, uint64_t>::iterator t = m_nextTravel.find(i->first);
            if (PlayerQuestMgr::HasActiveQuest(i->first))
                continue; // R7: quest chase owns the movement (travel fires when idle)
            uint64_t nowMs = m_elapsedTime;
            if (t == m_nextTravel.end())
            {
                // Newly online bot → schedule first travel 1-5h out.
                m_nextTravel[i->first] = (uint64_t)nowMs + (uint64_t)irand(
                    (int)(sPlayerbotAIConfig.travelMinSeconds * 1000),
                    (int)(sPlayerbotAIConfig.travelMaxSeconds * 1000));
                continue;
            }
            if (nowMs < t->second)
                continue;
            bool done = PlayerTravelMgr::DoTravel(bot);
            uint32 delaySec = done
                ? (uint32)irand((int)sPlayerbotAIConfig.travelMinSeconds, (int)sPlayerbotAIConfig.travelMaxSeconds)
                : 30; // deferred (combat/BG/group/anti-cam/no hub) → retry next sweep
            m_nextTravel[i->first] = (uint64_t)nowMs + (uint64_t)delaySec * 1000;
        }
        for (std::map<uint32, uint64_t>::iterator t = m_nextTravel.begin(); t != m_nextTravel.end();)
            if (!m_bots.count(t->first))
                t = m_nextTravel.erase(t);
            else
                ++t;
    }

    if (!((m_elapsedTime - m_lastUpdate) > confUpdateDiff))
        return; //Pas besoin d'update

    m_lastUpdate = m_elapsedTime;

    /* AC pattern: process async login queue */
    if (confAsyncLogin)
    {
        ProcessLoginQueue();
    }

    /* Connection des bots en attente (legacy sync path for non-async bots) */
    std::map<uint32, PlayerBotEntry*>::iterator iter;
    for (iter = m_bots.begin(); iter != m_bots.end(); ++iter)
    {
        if (!enable && !iter->second->customBot)
            continue;
        if (iter->second->state != PB_STATE_LOADING)
            continue;

        // Skip bots in async login queue (callback handles login)
        if (m_loadingBots.count(iter->second->playerGUID))
            continue;

        WorldSession* sess = sWorld.FindSession(iter->second->accountId);

        if (!sess)
        {
            continue;
        }

        if (iter->second->ai->OnSessionLoaded(iter->second, sess))
        {
            OnBotLogin(iter->second);
            m_stats.loadingCount--;

            if (iter->second->isChatBot)
                m_stats.onlineChat++;
            else
                m_stats.onlineCount++;
        }
        else
            sLog.outError("PLAYERBOT: Unable to load session id %u", iter->second->accountId);
    }

    if (!enable)
        return;

    uint32 updatesCount = (m_elapsedTime - m_lastBotsRefresh) / confBotsRefresh;
    for (uint32 i = 0; i < updatesCount; ++i)
    {
        AddOrRemoveBot();
        m_lastBotsRefresh += confBotsRefresh;
    }
}

/*
Toutes les X minutes, ajoute ou enleve un bot.
*/
bool PlayerBotMgr::AddOrRemoveBot()
{
    if (m_stats.onlineCount < confMinBots)
    {
        if (confAsyncLogin)
        {
            // Queue async login
            for (std::map<uint32, PlayerBotEntry*>::iterator it = m_bots.begin(); it != m_bots.end(); ++it)
            {
                if (it->second->state == PB_STATE_OFFLINE && !it->second->customBot)
                {
                    AddBotAsync((uint32)it->first);
                    return true;
                }
            }
        }
        return AddRandomBot();
    }
    if (m_stats.onlineCount > confMaxBots)
        return DeleteRandomBot();
    return false;
}

bool PlayerBotMgr::AddBot(PlayerBotAI* ai)
{
    // Find a correct accountid ?
    PlayerBotEntry* e = new PlayerBotEntry();
    e->ai = ai;
    e->accountId = GenBotAccountId();
    e->playerGUID = sObjectMgr.GeneratePlayerLowGuid();
    e->customBot = true;
    ai->botEntry = e;
    m_bots[e->playerGUID] = e;
    AddBot(e->playerGUID, false);
    return true;
}

bool PlayerBotMgr::AddBot(uint32 playerGUID, bool chatBot)
{
    uint32 accountId = 0;
    PlayerBotEntry *e = nullptr;
    std::map<uint32, PlayerBotEntry*>::iterator iter = m_bots.find(playerGUID);
    if (iter == m_bots.end())
        accountId = sObjectMgr.GetPlayerAccountIdByGUID(playerGUID);
    else
        accountId = iter->second->accountId;
    if (!accountId)
    {
        DETAIL_LOG("Compte du joueur %u introuvable ...", playerGUID);
        return false;
    }

    if (iter != m_bots.end())
        e = iter->second;
    else
    {
        DETAIL_LOG("Adding temporary PlayerBot.");
        e = new PlayerBotEntry();
        e->state        = PB_STATE_LOADING;
        e->playerGUID   = playerGUID;
        e->chance       = 10;
        e->accountId    = accountId;
        e->isChatBot    = chatBot;
        e->ai           = new PlayerBotAI(nullptr);
        m_bots[playerGUID] = e;
    }

    e->state = PB_STATE_LOADING;
    WorldSession *session = new WorldSession(accountId, nullptr, sAccountMgr.GetSecurity(accountId), 0, LOCALE_enUS, "<BOT>", 0);
    // Bots skip the normal auth handshake; create a dummy anticheat session so hooks are valid.
    BigNumber dummyKey(0);
    session->InitAntiCheatSession(&dummyKey);
    session->SetBot(e);
    sWorld.AddSession(session);
    m_stats.loadingCount++;

    if (chatBot)
        AddTempBot(accountId, 20000);

    return true;
}

// AC pattern: queue bot for async login
void PlayerBotMgr::AddBotAsync(uint32 playerGUID)
{
    // Check if already queued or loading
    if (m_loadingBots.count(playerGUID))
        return;

    PlayerBotEntry* e = nullptr;
    std::map<uint32, PlayerBotEntry*>::iterator iter = m_bots.find(playerGUID);
    if (iter == m_bots.end())
        return;

    e = iter->second;
    if (e->state != PB_STATE_OFFLINE)
        return;

    e->state = PB_STATE_LOADING;
    m_loginQueue.push_back(playerGUID);
    m_loadingBots.insert(playerGUID);
    m_stats.loadingCount++;

    if (confDebug)
        LOG_DEBUG("playerbots", "[PlayerBot][Queue] '%s' GUID:%u queued for async login (queue=%u, loading=%u)",
                  e->name.c_str(), playerGUID, (uint32)m_loginQueue.size(), (uint32)m_loadingBots.size());
}

// AC pattern: process login queue (async, non-blocking)
uint32 PlayerBotMgr::ProcessLoginQueue()
{
    uint32 processed = 0;
    uint32 batchSize = confAsyncBatchSize;

    while (!m_loginQueue.empty() && processed < batchSize)
    {
        uint32 playerGUID = m_loginQueue.back();
        m_loginQueue.pop_back();
        ++processed;

        PlayerBotEntry* e = nullptr;
        std::map<uint32, PlayerBotEntry*>::iterator iter = m_bots.find(playerGUID);
        if (iter == m_bots.end())
        {
            m_loadingBots.erase(playerGUID);
            m_stats.loadingCount--;
            continue;
        }
        e = iter->second;

        uint32 accountId = e->accountId;
        if (!accountId)
            accountId = sObjectMgr.GetPlayerAccountIdByGUID(playerGUID);
        if (!accountId)
        {
            sLog.outError("PLAYERBOT: Account ID not found for GUID %u", playerGUID);
            m_loadingBots.erase(playerGUID);
            m_stats.loadingCount--;
            continue;
        }

        // AC pattern: schedule login query, session created in callback
        ScheduleBotLogin(accountId, ObjectGuid(HIGHGUID_PLAYER, playerGUID));

        if (confDebug)
            LOG_DEBUG("playerbots", "[PlayerBot][AsyncLogin] '%s' GUID:%u login scheduled (processed=%u/%u)",
                      e->name.c_str(), playerGUID, processed, batchSize);
    }

    return processed;
}

bool PlayerBotMgr::AddRandomBot()
{
    uint32 alea = urand(0, totalChance);
    std::map<uint32, PlayerBotEntry*>::iterator it;
    bool done = false;
    for (it = m_bots.begin(); it != m_bots.end() && !done; it++)
    {
        if (it->second->state != PB_STATE_OFFLINE)
            continue;

        if (it->second->customBot)
            continue;

        uint32 chance = it->second->chance;

        if (chance >= alea)
        {
            if (confAsyncLogin)
                AddBotAsync((uint32)it->first);
            else
                AddBot(it->first);
            done = true;
        }

        alea -= chance;
    }

    return done;
}

void PlayerBotMgr::AddTempBot(uint32 account, uint32 time)
{
    m_tempBots[account] = time;
}

void PlayerBotMgr::RefreshTempBot(uint32 account)
{
    if (m_tempBots.find(account) != m_tempBots.end())
    {
        uint32& delay = m_tempBots[account];
        if (delay < 1000)
            delay = 1000;
    }
}

bool PlayerBotMgr::DeleteBot(uint32 playerGUID)
{
    std::map<uint32, PlayerBotEntry*>::iterator iter = m_bots.find(playerGUID);
    if (iter == m_bots.end())
        return false;

    if (iter->second->state == PB_STATE_LOADING)
        m_stats.loadingCount--;
    else if (iter->second->state == PB_STATE_ONLINE)
        m_stats.onlineCount--;

    OnBotLogout(iter->second);
    return true;
}

bool PlayerBotMgr::DeleteRandomBot()
{
    if (m_stats.onlineCount < 1)
        return false;

    uint32 idDelete = urand(0, m_stats.onlineCount);
    uint32 onlinePassed = 0;
    std::map<uint32, PlayerBotEntry*>::iterator iter;
    for (iter = m_bots.begin(); iter != m_bots.end(); iter++)
    {
        if (!iter->second->customBot && !iter->second->isChatBot && iter->second->state == PB_STATE_ONLINE)
        {
            onlinePassed++;
            if (onlinePassed == idDelete)
            {
                OnBotLogout(iter->second);
                m_stats.onlineCount--;
                return true;
            }
        }
    }

    return false;
}

bool PlayerBotMgr::ForceAccountConnection(WorldSession* sess)
{
    if (sess->GetBot())
        return sess->GetBot()->state != PB_STATE_OFFLINE;

    // Bots temporaires
    return m_tempBots.find(sess->GetAccountId()) != m_tempBots.end();
}

bool PlayerBotMgr::IsPermanentBot(uint32 playerGUID)
{
    std::map<uint32, PlayerBotEntry*>::iterator iter = m_bots.find(playerGUID);
    return iter != m_bots.end();
}

bool PlayerBotMgr::IsChatBot(uint32 playerGuid)
{
    std::map<uint32, PlayerBotEntry*>::iterator iter = m_bots.find(playerGuid);
    return iter != m_bots.end() && iter->second->isChatBot;
}

void PlayerBotMgr::AddAllBots()
{
    std::map<uint32, PlayerBotEntry*>::iterator it;
    for (it = m_bots.begin(); it != m_bots.end(); it++)
    {
        if (!it->second->isChatBot && it->second->state == PB_STATE_OFFLINE)
        {
            if (confAsyncLogin)
                AddBotAsync((uint32)it->first);
            else
                AddBot(it->first);
        }
    }
}

// AC pattern: rndbot stats — print bot activity readout
void PlayerBotMgr::PrintStats()
{
    uint32 online = 0, combat = 0, dead = 0, moving = 0;
    uint32 engine_noncombat = 0, engine_combat = 0, engine_dead = 0;
    std::map<uint8, uint32> perRace;
    std::map<uint8, uint32> perClass;
    std::map<uint8, uint32> lvlPerRace;
    std::map<uint8, uint32> lvlPerClass;
    uint8 maxBotLevel = 0;

    // Initialize race/class counters
    for (uint8 race = 0; race < 10; ++race)
    {
        perRace[race] = 0;
        lvlPerRace[race] = 0;
    }
    for (uint8 cls = 1; cls <= 9; ++cls)
    {
        perClass[cls] = 0;
        lvlPerClass[cls] = 0;
    }

    for (std::map<uint32, PlayerBotEntry*>::iterator i = m_bots.begin(); i != m_bots.end(); ++i)
    {
        PlayerBotEntry* e = i->second;
        if (e->state != PB_STATE_ONLINE)
            continue;

        online++;

        Player* bot = ObjectAccessor::FindPlayer(i->first);
        if (!bot)
            continue;

        maxBotLevel = std::max((uint8)maxBotLevel, (uint8)bot->GetLevel());

        uint8 race = bot->GetRace();
        uint8 cls = bot->GetClass();
        perRace[race]++;
        perClass[cls]++;
        lvlPerRace[race] += bot->GetLevel();
        lvlPerClass[cls] += bot->GetLevel();

        if (bot->IsInCombat())
            combat++;
        if (!bot->IsAlive())
            dead++;
        if (bot->IsMoving())
            moving++;

        // Check engine state
        PlayerbotAIBase* aiBase = dynamic_cast<PlayerbotAIBase*>(e->ai->engine);
        if (aiBase)
        {
            BotState state = aiBase->GetState();
            if (state == BOT_STATE_NON_COMBAT)
                engine_noncombat++;
            else if (state == BOT_STATE_COMBAT)
                engine_combat++;
            else
                engine_dead++;
        }
    }

    LOG_DEBUG("playerbots", "=== Playerbot Stats: %u online, %u total ===", online, (uint32)m_bots.size());
    LOG_DEBUG("playerbots", "  Queued: %u, Loading: %u", (uint32)m_loginQueue.size(), (uint32)m_loadingBots.size());
    LOG_DEBUG("playerbots", "  Combat: %u, Dead: %u, Moving: %u", combat, dead, moving);
    LOG_DEBUG("playerbots", "  Engine: non-combat=%u, combat=%u, dead=%u", engine_noncombat, engine_combat, engine_dead);

    LOG_DEBUG("playerbots", "Bots race:");
    const char* raceNames[] = {"Human","Orc","Dwarf","NightElf","Undead","Tauren","Gnome","Troll","Goblin","HighElf","!"};
    for (uint8 race = 0; race < 10; ++race)
    {
        if (perRace[race])
        {
            float avgLvl = (float)lvlPerRace[race] / perRace[race];
            LOG_DEBUG("playerbots", "  %-10s: %u, avg lvl: %.1f", raceNames[race], perRace[race], avgLvl);
        }
    }

    LOG_DEBUG("playerbots", "Bots class:");
    const char* classNames[] = {"","Warrior","Paladin","Hunter","Rogue","Priest","Shaman","Mage","Warlock","Druid"};
    for (uint8 cls = 1; cls <= 9; ++cls)
    {
        if (perClass[cls])
        {
            float avgLvl = (float)lvlPerClass[cls] / perClass[cls];
            LOG_DEBUG("playerbots", "  %-10s: %u, avg lvl: %.1f", classNames[cls], perClass[cls], avgLvl);
        }
    }

    LOG_DEBUG("playerbots", "Max bot level: %u", maxBotLevel);
}

// R6.1 agent interface: shared executor for one agent command.
// botName = player name (or "list" for the roster). cmd = free-form command
// understood by BotCommandAPI (state/move to x y z/attack guid/...). Returns
// the result string and logs an info.log audit line. Console `agent` branch
// and the file-command queue both call this.
std::string PlayerBotMgr::ExecAgentCommand(const char* botName, std::string const& cmd)
{
    if (!botName || !*botName)
        return "error: empty bot name";
    if (!*cmd.c_str())
        return "error: empty command";

    // Special roster command (no bot lookup).
    if (!strcasecmp(botName, "list"))
        return BotStateSnapshot::BuildBotsListJson();

    Player* bot = ObjectAccessor::FindPlayerByName(botName);
    if (!bot)
    {
        sLog.outInfo("playerbots: agent cmd failed: %s (player not found)", botName);
        return std::string("error: player not found: ") + botName;
    }
    std::string result = BotCommandAPI::Execute(bot, cmd);
    sLog.outInfo("playerbots: agent %s [%s] -> %s", bot->GetName(), cmd.c_str(), result.c_str());
    return result;
}

// R6.1 agent interface: file-command queue. The out-of-process agent drops a
// file at ../logs/botstate/commands/<bot>.cmd whose contents are the command
// text; we execute it here and write the reply to ../logs/botstate/results/
// <bot>.res. Bypasses the world console entirely (no pty/FIFO/GM needed).
// Gated by the caller (5s, PlayerBotMgr::Update).
void PlayerBotMgr::PollAgentCommandFiles()
{
    const char* cmdDir = "../logs/botstate/commands";
    const char* resDir = "../logs/botstate/results";
    static bool dirsMade = false;
    if (!dirsMade)
    {
        std::string mk = std::string("mkdir -p ") + cmdDir + " " + resDir;
        system(mk.c_str());
        dirsMade = true;
    }

    DIR* d = opendir(cmdDir);
    if (!d)
        return;

    struct dirent* ent;
    while ((ent = readdir(d)) != nullptr)
    {
        const char* fn = ent->d_name;
        size_t n = strlen(fn);
        if (n < 5 || strcmp(fn + n - 4, ".cmd") != 0)
            continue;

        std::string cmdPath = std::string(cmdDir) + "/" + fn;
        std::string botName(fn, n - 4);          // strip .cmd

        FILE* f = fopen(cmdPath.c_str(), "rb");
        if (!f)
            continue;
        char buf[4096];
        size_t got = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        buf[got] = '\0';

        // Trim trailing newlines/whitespace from the command text.
        while (got > 0 && (buf[got-1] == '\n' || buf[got-1] == '\r' || buf[got-1] == ' '))
            buf[--got] = '\0';

        std::string cmd(buf);
        std::string result = ExecAgentCommand(botName.c_str(), cmd);

        std::string resPath = std::string(resDir) + "/" + botName + ".res";
        FILE* rf = fopen(resPath.c_str(), "wb");
        if (rf)
        {
            fwrite(result.data(), 1, result.size(), rf);
            fputc('\n', rf);
            fclose(rf);
        }

        // Consume the command file (best-effort; ignore errors).
        remove(cmdPath.c_str());
    }
    closedir(d);
}

// AC pattern: HandleConsoleCommand — ".playerbots rndbot stats" etc.
bool PlayerBotMgr::HandleConsoleCommand(ChatHandler* handler, char* args)
{
    if (!args || !*args)
    {
        handler->PSendSysMessage("Usage: .playerbots <rndbot|pmon|bot> [subcommand]");
        handler->PSendSysMessage("  .playerbots rndbot stats  - Show bot activity stats");
        handler->PSendSysMessage("  .playerbots pmon [tick|reset|toggle] - Performance monitor");
        handler->PSendSysMessage("  .playerbots bot list       - List all bots");
        return false;
    }

    // Parse first subcommand
    char* cmd = strtok(args, " ");
    char* subcmd = strtok(nullptr, " ");

    if (!strcmp(cmd, "rndbot"))
    {
        if (!subcmd || !strcmp(subcmd, "stats"))
        {
            sPlayerBotMgr.PrintStats();
            return true;
        }
        handler->PSendSysMessage("Usage: .playerbots rndbot stats");
        return false;
    }

    if (!strcmp(cmd, "pmon"))
    {
        if (!subcmd)
        {
            // Default: print total stats
            sPlayerbotPerfMonitor.PrintStats(false, false);
            return true;
        }
        if (!strcmp(subcmd, "tick"))
        {
            sPlayerbotPerfMonitor.PrintStats(true, false);
            return true;
        }
        if (!strcmp(subcmd, "reset"))
        {
            sPlayerbotPerfMonitor.Reset();
            handler->PSendSysMessage("Performance monitor reset.");
            return true;
        }
        if (!strcmp(subcmd, "toggle"))
        {
            sPlayerbotAIConfig.perfMonEnabled = !sPlayerbotAIConfig.perfMonEnabled;
            handler->PSendSysMessage(sPlayerbotAIConfig.perfMonEnabled ? "Performance monitor enabled." : "Performance monitor disabled.");
            return true;
        }
        if (!strcmp(subcmd, "stack"))
        {
            sPlayerbotPerfMonitor.PrintStats(false, true);
            return true;
        }
        handler->PSendSysMessage("Usage: .playerbots pmon [tick|reset|toggle|stack]");
        return false;
    }

    // R6.1 agent interface: .playerbots agent <botName> <command...>
    // Free-form command handled by BotCommandAPI (state/move/attack/cast/...).
    // Also: .playerbots agent list  -> compact bots list JSON
    if (!strcmp(cmd, "agent"))
    {
        if (!subcmd)
        {
            handler->PSendSysMessage("Usage: .playerbots agent <botName|list> [command...]");
            handler->PSendSysMessage("  agent list               - compact all-bots JSON");
            handler->PSendSysMessage("  agent <bot> <command>    - state|move to x y z|attack guid|cast id [guid]|loot|interact guid|accept id [giver]|turnin id [giver]|drop id|say text|stop|engine 0|1|2|bots");
            return false;
        }
        if (!strcmp(subcmd, "list"))
        {
            handler->PSendSysMessage(BotStateSnapshot::BuildBotsListJson().c_str());
            return true;
        }
        // Everything after the bot name is the command. Re-derive it from the
        // original args (strtok state already consumed by the cmd/subcmd parse;
        // strtok(nullptr, "") would be UB).
        char* rest = args;
        rest = strchr(rest, ' ');               // after "agent"
        if (rest)
        {
            rest++;
            while (*rest == ' ') rest++;
        }
        rest = rest ? strchr(rest, ' ') : nullptr;   // after the bot name
        if (rest)
        {
            rest++;
            while (*rest == ' ') rest++;
        }
        if (!rest || !*rest)
        {
            handler->PSendSysMessage("Usage: .playerbots agent <botName> <command...>");
            return false;
        }
        // Shared executor (console + file queue): lookup, execute, audit-log.
        std::string result = sPlayerBotMgr.ExecAgentCommand(subcmd, rest);
        handler->PSendSysMessage(result.c_str());
        return true;
    }

    if (!strcmp(cmd, "bot"))
    {
        if (!subcmd || !strcmp(subcmd, "list"))
        {
            uint32 online = 0, offline = 0;
            for (std::map<uint32, PlayerBotEntry*>::iterator i = sPlayerBotMgr.m_bots.begin();
                 i != sPlayerBotMgr.m_bots.end(); ++i)
            {
                if (i->second->state == PB_STATE_ONLINE)
                    online++;
                else
                    offline++;
            }
            handler->PSendSysMessage("Bots: %u online, %u offline, %u total", online, offline, (uint32)sPlayerBotMgr.m_bots.size());
            return true;
        }
        handler->PSendSysMessage("Usage: .playerbots bot list");
        return false;
    }

    handler->PSendSysMessage("Unknown command: %s", cmd);
    handler->PSendSysMessage("Usage: .playerbots <rndbot|pmon|bot> [subcommand]");
    return false;
}
