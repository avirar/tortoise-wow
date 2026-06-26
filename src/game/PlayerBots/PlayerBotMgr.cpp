#include "Common.h"
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
#include "Bot/PlayerbotAIBase.h"
#include "Util/PlayerbotAIConfig.h"
#include "Util/PerfMonitor.h"
#include "Anticheat.h"
#include "Log.h"
#include "Logging.h"

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
    }
    else
    {
        LOG_DEBUG("playerbots", "[OnPlayerInWorld] No bot entry for player '%s'", player->GetName());
    }
}

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
