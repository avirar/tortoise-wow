/*
 * PlayerQuestMgr — R7: real quest pipeline (see header for design + data).
 */
#include "PlayerQuestMgr.h"

#include "Common.h"
#include "Log.h"
#include "Object.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "Creature.h"
#include "Unit.h"
#include "ObjectMgr.h"
#include "QuestDef.h"
#include "AiObjectContext.h"
#include "PlayerBotMgr.h"
#include "Maps/Map.h"
#include "Maps/MapManager.h"
#include "Maps/GridMap.h"
#include "Maps/CellImpl.h"
#include "Maps/GridNotifiers.h"
#include "Database/DatabaseEnv.h"
#include "Protocol/Opcodes.h"
#include "PlayerbotAIConfig.h"
#include "PlayerBotMgr.h"
#include "ServerFacade.h"
#include "PlayerBotAI.h"
#include "Value/Value.h"
#include "Mgr/Item/StatsWeightCalculator.h"

// Statics
bool PlayerQuestMgr::s_loaded = false;
std::map<uint32, std::vector<PlayerQuestMgr::QuestDest> > PlayerQuestMgr::s_takers;
std::map<uint32, std::map<uint8, std::vector<PlayerQuestMgr::QuestDest> > > PlayerQuestMgr::s_objectives;
std::set<uint32> PlayerQuestMgr::s_giverEntries;
std::map<uint32, PlayerQuestMgr::BotQuestState> PlayerQuestMgr::s_states;

/*
 * One-time load: takers (creature_involvedrelation ⋈ creature), kill-objective
 * POIs (quest_template.ReqCreatureOrGOId1-4 ⋈ creature), giver entries
 * (creature_questrelation ⋈ creature). Maps 0/1 only (overworld v1).
 */
void PlayerQuestMgr::Load()
{
    if (s_loaded)
        return;
    s_loaded = true; // re-entry guard (travel mgr pattern)

    // Takers: quest → spawn positions of the involved-relation NPCs.
    QueryResult* result = WorldDatabase.PQuery(
        "SELECT q.quest, c.map, c.position_x, c.position_y, c.position_z, c.id "
        "FROM creature_involvedrelation q "
        "JOIN creature c ON c.id = q.id "
        "WHERE c.map IN (0, 1)");
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            QuestDest d;
            d.map = f[1].GetUInt32();
            d.x = f[2].GetFloat();
            d.y = f[3].GetFloat();
            d.z = f[4].GetFloat();
            d.entry = f[5].GetUInt32();
            s_takers[f[0].GetUInt32()].push_back(d);
        } while (result->NextRow());
        delete result;
    }

    // Kill-objective POIs: quest → objIdx → spawn positions of the kill entry.
    // (GO objectives are rare in the overworld pool — v1 is creatures only.)
    result = WorldDatabase.PQuery(
        "SELECT t.quest, t.objidx, c.map, c.position_x, c.position_y, c.position_z, c.id "
        "FROM (SELECT id AS quest, 1 AS objidx, ReqCreatureOrGOId1 AS entry FROM quest_template WHERE ReqCreatureOrGOId1 > 0 "
        "      UNION ALL SELECT id, 2, ReqCreatureOrGOId2 FROM quest_template WHERE ReqCreatureOrGOId2 > 0 "
        "      UNION ALL SELECT id, 3, ReqCreatureOrGOId3 FROM quest_template WHERE ReqCreatureOrGOId3 > 0 "
        "      UNION ALL SELECT id, 4, ReqCreatureOrGOId4 FROM quest_template WHERE ReqCreatureOrGOId4 > 0) t "
        "JOIN creature c ON c.id = t.entry "
        "WHERE c.map IN (0, 1)");
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            QuestDest d;
            d.map = f[2].GetUInt32();
            d.x = f[3].GetFloat();
            d.y = f[4].GetFloat();
            d.z = f[5].GetFloat();
            d.entry = f[6].GetUInt32();
            s_objectives[f[0].GetUInt32()][(uint8)f[1].GetUInt32()].push_back(d);
        } while (result->NextRow());
        delete result;
    }

    // Giver entries (for the nearby-giver proximity scan).
    result = WorldDatabase.PQuery(
        "SELECT DISTINCT c.id FROM creature c "
        "JOIN creature_questrelation q ON q.id = c.id "
        "WHERE c.map IN (0, 1)");
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            s_giverEntries.insert(f[0].GetUInt32());
        } while (result->NextRow());
        delete result;
    }

    sLog.outInfo("playerbots: quest cache loaded: %u taker quests, %u objective quests, %u giver entries (overworld)",
        GetTakerCount(), GetObjectiveQuestCount(), GetGiverEntryCount());
}

/*
 * Cell-scan visitor: nearest live creature of a given entry within range.
 */
struct NpcFinderVisitor
{
    Unit const* me;
    uint32 entry;
    float range;
    Creature* best;
    float bestDist;

    NpcFinderVisitor() : me(nullptr), entry(0), range(0), best(nullptr), bestDist(0) {}
    void Init(Unit const* m, uint32 e, float r) { me = m; entry = e; range = r; best = nullptr; bestDist = 0; }

    void CheckCreature(Creature* cre)
    {
        if (cre->GetEntry() != entry || !cre->IsAlive())
            return;
        float d = me->GetDistance(cre);
        if (d <= range && (best == nullptr || d < bestDist))
        {
            best = cre;
            bestDist = d;
        }
    }

    void Visit(PlayerMapType &m) {}
    void Visit(CreatureMapType &m)
    {
        for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            CheckCreature(itr->getSource());
    }
    template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    template<class NOT_INTERESTED> void Visit(NOT_INTERESTED &) {}
};

Creature* PlayerQuestMgr::FindLiveNpcNear(Player* bot, uint32 entry, float range)
{
    Map* map = bot->GetMap();
    if (!map)
        return nullptr;

    NpcFinderVisitor finder;
    finder.Init(bot, entry, range);

    CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();

    TypeContainerVisitor<NpcFinderVisitor, WorldTypeMapContainer> world_vis(finder);
    TypeContainerVisitor<NpcFinderVisitor, GridTypeMapContainer> grid_vis(finder);
    cell.Visit(p, world_vis, *map, *bot, range);
    cell.Visit(p, grid_vis, *map, *bot, range);
    return finder.best;
}

/*
 * AC-style POI distance filter: same map + same zone + within maxDist.
 */
static bool PoiMatchesZone(uint32 mapId, float x, float y, float z, float maxDist,
                           uint32 botMap, float botX, float botY, float botZ)
{
    if (mapId != botMap)
        return false;
    float dx = x - botX;
    float dy = y - botY;
    if (dx * dx + dy * dy > maxDist * maxDist)
        return false;
    // Zone equality (AC: same area — prevents cross-zone POI picks).
    uint32 botArea = sTerrainMgr.GetAreaId(botMap, botX, botY, botZ);
    uint32 poiArea = sTerrainMgr.GetAreaId(mapId, x, y, z);
    return botArea == poiArea;
}

/*
 * Kill-objective progress (sum of CreatureOrGOCount over kill objectives).
 * Drives the no-progress abandon clock.
 */
uint32 PlayerQuestMgr::KillProgress(Player* bot, Quest const* quest)
{
    QuestStatusMap const& qmap = bot->getQuestStatusMap();
    QuestStatusMap::const_iterator it = qmap.find(quest->GetQuestId());
    if (it == qmap.end())
        return 0;
    QuestStatusData const& sd = it->second;
    uint32 sum = 0;
    for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        if (quest->ReqCreatureOrGOId[i] > 0)
            sum += sd.m_creatureOrGOcount[i];
    return sum;
}

/*
 * Acceptability filter (AC IsQuestWorthDoing + IsQuestCapableDoing, v1 solo).
 */
bool PlayerQuestMgr::WorthAccepting(Player const* bot, Quest const* quest)
{
    if (!quest)
        return false;
    uint32 botLvl = bot->GetLevel();

    // Solo, normal, non-repeatable (AC NewRpgInfo::IsQuestCapableDoing rules).
    if (quest->GetType() != 0)
        return false;
    if (quest->GetSuggestedPlayers() >= 2)
        return false;
    if (quest->IsRepeatable())
        return false;
    if (bot->GetQuestStatus(quest->GetQuestId()) != QUEST_STATUS_NONE)
        return false;

    // Level band: travel-matched ±2 (quest level proxy for the bot).
    uint32 qLvl = quest->GetQuestLevel();
    if (qLvl == 0)
        qLvl = quest->GetMinLevel();
    if (qLvl < botLvl - 2 || qLvl > botLvl + 2)
        return false;

    // Engine preconditions (level reqs, rep, faction, log space).
    if (!bot->CanTakeQuest(quest, false))
        return false;
    if (!bot->CanAddQuest(quest, false))
        return false;

    // v1: every kill objective needs a known overworld POI (else the bot
    // could never complete it → would churn the no-progress abandon).
    for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
    {
        if (quest->ReqCreatureOrGOId[i] > 0)
        {
            std::map<uint32, std::map<uint8, std::vector<QuestDest> > >::const_iterator q =
                s_objectives.find(quest->GetQuestId());
            if (q == s_objectives.end() || q->second.find(i) == q->second.end())
                return false;
        }
    }
    return true;
}

/*
 * Quest-log hygiene (AC NewRpgInfo::OrganizeQuestLog, v1: no zone rule).
 * Drops until free slots >= minFree: FAILED first, then level-mismatched,
 * then non-capable (too hard / party / elite). Returns drops performed.
 */
static bool DropBySlot(Player* bot, uint8 slot)
{
    WorldPacket pkt(CMSG_QUESTLOG_REMOVE_QUEST);
    pkt << (uint8)slot;
    bot->GetSession()->HandleQuestLogRemoveQuest(pkt);
    return true;
}

void PlayerQuestMgr::OrganizeQuestLog(Player* bot, BotQuestState& st)
{
    uint32 freeSlots = 0;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
        if (!bot->GetQuestSlotQuestId(slot))
            ++freeSlots;
    if (freeSlots >= sPlayerbotAIConfig.questLogMinFreeSlots)
        return;

    uint32 botLvl = bot->GetLevel();
    // Two passes: strict drops (FAILED / far below), then softer (too hard).
    for (int pass = 0; pass < 2 && freeSlots < sPlayerbotAIConfig.questLogMinFreeSlots; ++pass)
    {
        for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
        {
            if (freeSlots >= sPlayerbotAIConfig.questLogMinFreeSlots)
                break;
            uint32 qid = bot->GetQuestSlotQuestId(slot);
            if (!qid)
                continue;
            Quest const* quest = sObjectMgr.GetQuestTemplate(qid);
            if (!quest)
                continue;
            QuestStatus status = bot->GetQuestStatus(qid);
            uint32 qLvl = quest->GetQuestLevel();
            if (qLvl == 0)
                qLvl = quest->GetMinLevel();

            bool drop = false;
            if (pass == 0)
            {
                drop = (status == QUEST_STATUS_FAILED)
                    || (qLvl > 0 && botLvl > qLvl + 5); // far below = dead weight
            }
            else
            {
                drop = (qLvl > 0 && qLvl > botLvl + 3)   // too hard (AC capable rule)
                    || quest->GetType() != 0
                    || quest->GetSuggestedPlayers() >= 2;
            }
            if (drop)
            {
                sLog.outInfo("playerbots: %s dropped quest %u '%s' (log organize, pass %d, status %u)",
                    bot->GetName(), qid, quest->GetTitle().c_str(), pass, (uint32)status);
                DropBySlot(bot, slot);
                ++freeSlots;
            }
        }
    }
}

/*
 * Pick the nearest in-band objective POI for one or more incomplete kill
 * objectives. Writes st dest + baseline. false = no usable POI.
 */
bool PlayerQuestMgr::PickObjectivePoi(Player* bot, uint32 questId, BotQuestState& st)
{
    Quest const* quest = sObjectMgr.GetQuestTemplate(questId);
    if (!quest)
        return false;
    QuestStatusMap const& qmap = bot->getQuestStatusMap();
    QuestStatusMap::const_iterator it = qmap.find(questId);
    if (it == qmap.end() || it->second.m_status != QUEST_STATUS_INCOMPLETE)
        return false;
    QuestStatusData const& sd = it->second;

    std::map<uint32, std::map<uint8, std::vector<QuestDest> > >::const_iterator q =
        s_objectives.find(questId);
    if (q == s_objectives.end())
        return false;

    float maxDist = (float)sPlayerbotAIConfig.questPoiMaxDist;
    uint32 botMap = bot->GetMap() ? bot->GetMap()->GetId() : 0;
    float bx = bot->GetPositionX(), by = bot->GetPositionY(), bz = bot->GetPositionZ();

    float bestDist2 = 0;
    bool found = false;
    for (std::map<uint8, std::vector<QuestDest> >::const_iterator obj = q->second.begin();
         obj != q->second.end(); ++obj)
    {
        uint8 idx = obj->first;
        // Only incomplete kill objectives (AC ActiveQuestObjectivesValue).
        if (!(quest->ReqCreatureOrGOId[idx] > 0 && quest->ReqCreatureOrGOCount[idx] > sd.m_creatureOrGOcount[idx]))
            continue;
        for (std::vector<QuestDest>::const_iterator i = obj->second.begin(); i != obj->second.end(); ++i)
        {
            QuestDest const& d = *i;
            if (!PoiMatchesZone(d.map, d.x, d.y, d.z, maxDist, botMap, bx, by, bz))
                continue;
            float dx = d.x - bx, dy = d.y - by;
            float dist2 = dx * dx + dy * dy;
            if (!found || dist2 < bestDist2)
            {
                found = true;
                bestDist2 = dist2;
                st.destMap = d.map;
                st.destX = d.x;
                st.destY = d.y;
                st.destZ = d.z;
                st.poiType = 1;
                st.reachTs = 0;
                st.stateTs = time(nullptr);
            }
        }
    }
    return found;
}

/*
 * Pick the nearest taker for a completed quest.
 */
bool PlayerQuestMgr::PickTakerDest(Player* bot, uint32 questId, BotQuestState& st)
{
    std::map<uint32, std::vector<QuestDest> >::const_iterator t = s_takers.find(questId);
    if (t == s_takers.end() || t->second.empty())
        return false;

    float maxDist = (float)sPlayerbotAIConfig.questPoiMaxDist;
    uint32 botMap = bot->GetMap() ? bot->GetMap()->GetId() : 0;
    float bx = bot->GetPositionX(), by = bot->GetPositionY(), bz = bot->GetPositionZ();

    float bestDist2 = 0;
    bool found = false;
    for (std::vector<QuestDest>::const_iterator i = t->second.begin(); i != t->second.end(); ++i)
    {
        QuestDest const& d = *i;
        if (!PoiMatchesZone(d.map, d.x, d.y, d.z, maxDist, botMap, bx, by, bz))
            continue;
        float dx = d.x - bx, dy = d.y - by;
        float dist2 = dx * dx + dy * dy;
        if (!found || dist2 < bestDist2)
        {
            found = true;
            bestDist2 = dist2;
            st.destMap = d.map;
            st.destX = d.x;
            st.destY = d.y;
            st.destZ = d.z;
            st.poiType = 2;
            st.reachTs = 0;
            st.stateTs = time(nullptr);
        }
    }
    return found;
}

/*
 * Teleport to the current POI (R5e pattern: ground z, motion/target clear,
 * interruptible auras, fresh grind-grace).
 */
void PlayerQuestMgr::TeleportToPoi(Player* bot, BotQuestState const& st)
{
    if (bot->IsInCombat())
        return; // no mid-fight blinks (stale-combat breaker handles the stuck case)
    uint32 destMap = st.destMap;
    float x = st.destX + (float)((int)urand(0, 30) - 15);
    float y = st.destY + (float)((int)urand(0, 30) - 15);
    float z = st.destZ;

    Map* targetMap = sMapMgr.FindMap(destMap);
    if (!targetMap)
        return;
    float ground = targetMap->GetHeight(x, y, z);
    z = (ground <= INVALID_HEIGHT) ? (z + 0.05f) : (ground + 0.05f);
    if (!MapManager::IsValidMapCoord(destMap, x, y, z))
        return;

    // R5e teleport sequence (shared with travel).
    bot->GetMotionMaster()->Clear();
    if (PlayerBotEntry* e = sPlayerBotMgr.GetBot(bot->GetObjectGuid().GetCounter()))
        if (e && e->ai)
            if (AiObjectContext* ctx = e->ai->GetAiObjectContext())
                ctx->GetValue<Unit*>("current target")->Set(nullptr);
    bot->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED | AURA_INTERRUPT_FLAG_CHANGE_MAP);

    if (!bot->TeleportTo(destMap, x, y, z, 0.0f))
        return;

    ServerFacade::MarkViableGrindTargetSeen(bot); // fresh relocation grace
}

/*
 * Interaction primitives — CMSG replay through the bot's own session
 * (handlers re-verify CanInteractWithQuestGiver / quest state).
 */
bool PlayerQuestMgr::AcceptQuest(Player* bot, ObjectGuid const& giverGuid, uint32 questId)
{
    if (!bot->GetSession())
        return false;
    Quest const* quest = sObjectMgr.GetQuestTemplate(questId);
    if (!quest)
        return false;
    WorldPacket pkt(CMSG_QUESTGIVER_ACCEPT_QUEST);
    pkt << giverGuid << (uint32)questId;
    bot->GetSession()->HandleQuestgiverAcceptQuestOpcode(pkt);
    sLog.outInfo("playerbots: %s accepted quest %u '%s' (lvl %u)",
        bot->GetName(), questId, quest->GetTitle().c_str(), bot->GetLevel());
    return true;
}

bool PlayerQuestMgr::TurnInQuest(Player* bot, ObjectGuid const& takerGuid, uint32 questId)
{
    if (!bot->GetSession())
        return false;
    Quest const* quest = sObjectMgr.GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Best reward choice (R3a integration): score the RewChoiceItemId pool
    // with the StatsWeightCalculator; ties → first. 0 choices → index 0.
    uint32 bestIdx = 0;
    float bestScore = -1;
    for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT && i < quest->GetRewChoiceItemsCount(); ++i)
    {
        uint32 itemId = quest->RewChoiceItemId[i];
        if (!itemId)
            continue;
        float score = StatsWeightCalculator(bot).CalculateItem(itemId);
        if (score > bestScore)
        {
            bestScore = score;
            bestIdx = i;
        }
    }

    WorldPacket pkt(CMSG_QUESTGIVER_CHOOSE_REWARD);
    pkt << takerGuid << (uint32)questId << (uint32)bestIdx;
    bot->GetSession()->HandleQuestgiverChooseRewardOpcode(pkt);
    sLog.outInfo("playerbots: %s turned in quest %u '%s' (reward choice %u)",
        bot->GetName(), questId, quest->GetTitle().c_str(), bestIdx);
    return true;
}

bool PlayerQuestMgr::DropQuest(Player* bot, uint32 questId)
{
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        if (bot->GetQuestSlotQuestId(slot) == questId)
        {
            WorldPacket pkt(CMSG_QUESTLOG_REMOVE_QUEST);
            pkt << (uint8)slot;
            bot->GetSession()->HandleQuestLogRemoveQuest(pkt);
            return true;
        }
    }
    return false;
}

/*
 * Nearby quest-giver hunt (idle bots only): scan creatures within
 * questAcceptRadius for live givers, PrepareQuestMenu each (in-memory),
 * accept the first WorthAccepting quest. One accept per sweep.
 */
struct GiverScannerVisitor
{
    Unit const* me;
    std::set<uint32> const* giverEntries;
    float range;
    std::vector<Creature*> candidates;

    GiverScannerVisitor() : me(nullptr), giverEntries(nullptr), range(0) {}
    void Init(Unit const* m, std::set<uint32> const* ge, float r)
    {
        me = m; giverEntries = ge; range = r; candidates.clear();
    }

    void CheckCreature(Creature* cre)
    {
        if (!giverEntries->count(cre->GetEntry()) || !cre->IsAlive())
            return;
        if (me->GetDistance(cre) <= range)
            candidates.push_back(cre);
    }

    void Visit(PlayerMapType &m) {}
    void Visit(CreatureMapType &m)
    {
        for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            CheckCreature(itr->getSource());
    }
    template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    template<class NOT_INTERESTED> void Visit(NOT_INTERESTED &) {}
};

/*
 * Full per-bot decision step (30s cadence from PlayerBotMgr::Update).
 */
void PlayerQuestMgr::ProcessBot(Player* bot)
{
    if (!bot || !bot->IsAlive() || bot->IsBeingTeleported() || bot->InBattleGround())
        return;
    Map* map = bot->GetMap();
    if (!map || map->IsDungeon())
        return;

    uint32 guid = bot->GetObjectGuid().GetCounter();
    BotQuestState& st = s_states[guid];
    time_t now = time(nullptr);

    // --- 0. Validate active quest still exists in the log with usable state.
    if (st.questId != 0)
    {
        Quest const* quest = sObjectMgr.GetQuestTemplate(st.questId);
        if (!quest)
        {
            st = BotQuestState();
            return;
        }
        QuestStatusMap const& qmap = bot->getQuestStatusMap();
        QuestStatusMap::const_iterator it = qmap.find(st.questId);
        if (it == qmap.end() || it->second.m_status == QUEST_STATUS_NONE)
        {
            // Dropped / already rewarded elsewhere → clear.
            st = BotQuestState();
        }
        else if (it->second.m_status == QUEST_STATUS_INCOMPLETE && it->second.m_status != QUEST_STATUS_COMPLETE)
        {
            // No-progress abandon (AC lowPriorityQuest 5-min rule): at the
            // POI for questNoProgressSeconds with zero kill-objective gain.
            if (st.poiType == 1 && st.reachTs > 0 &&
                (uint32)(now - st.reachTs) >= sPlayerbotAIConfig.questNoProgressSeconds)
            {
                uint32 progress = KillProgress(bot, quest);
                if (progress <= st.progressBaseline)
                {
                    sLog.outInfo("playerbots: %s abandoned quest %u '%s' (no progress %us at POI)",
                        bot->GetName(), st.questId, quest->GetTitle().c_str(),
                        (uint32)(now - st.reachTs));
                    st.abandoned.insert(st.questId);
                    DropQuest(bot, st.questId);
                    st = BotQuestState();
                    return;
                }
                st.progressBaseline = progress; // progress made → extend the clock
                st.reachTs = now;
            }
        }
    }

    // --- 1. COMPLETE → taker + turn-in.
    if (st.questId != 0)
    {
        Quest const* quest = sObjectMgr.GetQuestTemplate(st.questId);
        if (quest)
        {
            QuestStatusMap const& qmap = bot->getQuestStatusMap();
            QuestStatusMap::const_iterator it = qmap.find(st.questId);
            if (it != qmap.end() && it->second.m_status == QUEST_STATUS_COMPLETE && !it->second.m_rewarded)
            {
                if (st.poiType != 2 || !st.destMap)
                {
                    if (!PickTakerDest(bot, st.questId, st))
                    {
                        // No overworld taker in range (dungeon turn-in?) —
                        // keep the quest, let it age; travel may bring us there.
                        st.poiType = 0;
                        return;
                    }
                }

                float dist = bot->GetDistance2d(st.destX, st.destY);
                if (dist > 20.0f)
                {
                    TeleportToPoi(bot, st);
                    return;
                }

                // At the taker: find the live NPC, verify the menu, turn in.
                // (destEntry not tracked in v1 — scan all taker entries for
                // this quest via the cache.)
                std::map<uint32, std::vector<QuestDest> >::const_iterator t = s_takers.find(st.questId);
                if (t != s_takers.end())
                {
                    Creature* npc = nullptr;
                    for (std::vector<QuestDest>::const_iterator i = t->second.begin(); i != t->second.end(); ++i)
                    {
                        if (i->map != st.destMap)
                            continue;
                        npc = FindLiveNpcNear(bot, i->entry, 25.0f);
                        if (npc)
                            break;
                    }
                    if (npc)
                    {
                        bot->PrepareQuestMenu(npc->GetObjectGuid());
                        if (bot->PlayerTalkClass &&
                            bot->PlayerTalkClass->GetQuestMenu().HasItem(st.questId) &&
                            bot->CanRewardQuest(quest, false))
                        {
                            TurnInQuest(bot, npc->GetObjectGuid(), st.questId);
                            st = BotQuestState(); // next sweep: accept here
                            return;
                        }
                    }
                }
                // No live taker / menu miss: retry next sweep (don't teleport again).
                st.poiType = 0;
                return;
            }
        }
    }

    // --- 2. INCOMPLETE → objective POI (move, then grind handles kills).
    if (st.questId != 0)
    {
        if (st.poiType == 0 || !st.destMap)
        {
            if (!PickObjectivePoi(bot, st.questId, st))
            {
                // POI went out of band (bot wandered) → re-pick each sweep;
                // if never pickable the no-progress clock will abandon it.
                st.poiType = 0;
                return;
            }
        }
        float dist = bot->GetDistance2d(st.destX, st.destY);
        if (dist > 20.0f)
        {
            TeleportToPoi(bot, st);
            return;
        }
        // At the POI: start/hold the no-progress clock, then grind (the
        // NON_COMBAT grinding strategy kills the quest mobs as normal).
        if (st.reachTs == 0)
        {
            st.reachTs = now;
            Quest const* quest = sObjectMgr.GetQuestTemplate(st.questId);
            st.progressBaseline = quest ? KillProgress(bot, quest) : 0;
        }
        return;
    }

    // --- 3. Idle: organize the log, then hunt for a nearby giver.
    OrganizeQuestLog(bot, st);

    Map* botMap = bot->GetMap();
    if (!botMap)
        return;

    GiverScannerVisitor scanner;
    scanner.Init(bot, &s_giverEntries, (float)sPlayerbotAIConfig.questAcceptRadius);
    CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();
    TypeContainerVisitor<GiverScannerVisitor, WorldTypeMapContainer> world_vis(scanner);
    TypeContainerVisitor<GiverScannerVisitor, GridTypeMapContainer> grid_vis(scanner);
    cell.Visit(p, world_vis, *botMap, *bot, (float)sPlayerbotAIConfig.questAcceptRadius);
    cell.Visit(p, grid_vis, *botMap, *bot, (float)sPlayerbotAIConfig.questAcceptRadius);

    for (std::vector<Creature*>::const_iterator i = scanner.candidates.begin();
         i != scanner.candidates.end(); ++i)
    {
        Creature* giver = *i;
        if (!giver || !giver->IsAlive())
            continue;
        bot->PrepareQuestMenu(giver->GetObjectGuid());
        if (!bot->PlayerTalkClass || bot->PlayerTalkClass->GetQuestMenu().Empty())
            continue;
        for (uint8 idx = 0; idx < bot->PlayerTalkClass->GetQuestMenu().MenuItemCount(); ++idx)
        {
            uint32 qid = bot->PlayerTalkClass->GetQuestMenu().GetItem(idx).m_qId;
            Quest const* quest = sObjectMgr.GetQuestTemplate(qid);
            if (!quest || st.abandoned.count(qid))
                continue;
            if (!WorthAccepting(bot, quest))
                continue;
            if (AcceptQuest(bot, giver->GetObjectGuid(), qid))
            {
                st.questId = qid;
                st.poiType = 0; // next sweep picks the objective POI
                st.stateTs = now;
                return; // one accept per sweep
            }
        }
    }
}

/*
 * True while the bot has a quest to chase (travel/relocation skip this).
 */
bool PlayerQuestMgr::HasActiveQuest(uint32 guidCounter)
{
    std::map<uint32, BotQuestState>::const_iterator it = s_states.find(guidCounter);
    return it != s_states.end() && it->second.questId != 0;
}
