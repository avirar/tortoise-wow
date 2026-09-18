/*
 * PlayerQuestMgr — R7: real quest pipeline (AC mod-playerbots base quest
 * layer + NewRPG quest state machine, ported as a rewrite per master plan A5).
 *
 * Bots take real quests in towns, travel to objectives, complete them
 * (existing grinding does the kills), turn in at takers, and repeat.
 * No WorldPacketTrigger in this port → AC's gossip/complete-quest packet
 * triggers are replaced by PrepareQuestMenu() polling (in-memory, no
 * distance check) + proximity-gated CMSG replay through the bot's own
 * WorldSession handlers (which re-verify CanInteractWithQuestGiver).
 *
 * Data (verified against tw_world, 2026-09-18):
 *  - takers:  creature_involvedrelation (6342 rows) ⋈ creature, maps 0/1
 *  - objectives: quest_template.ReqCreatureOrGOId1-4 (>0 creature, 885
 *    quests have kill objectives) ⋈ creature spawns, maps 0/1
 *  - givers:  creature_questrelation ⋈ creature, maps 0/1 (same source as
 *    the travel POI cache)
 *  - reward choice: quest_template.RewChoiceItemId1-6 (custom column names
 *    in this core; QUEST_REWARD_CHOICES_COUNT)
 *
 * v1 scope (master plan R7): solo (SuggestedPlayers<2), Type=0 (normal)
 * quests only, kill-objective POIs only (item objectives = Phase 2 via
 * item_loot_template), one active quest per bot, teleport movement
 * (walking = R6/TravelSystem port).
 */
#ifndef _PLAYERBOT_QUEST_MGR_H
#define _PLAYERBOT_QUEST_MGR_H

#include "Common.h"
#include <map>
#include <set>
#include <vector>
#include <ctime>

class Player;
class Quest;

class PlayerQuestMgr
{
public:
    static PlayerQuestMgr& getInstance() { static PlayerQuestMgr inst; return inst; }

    struct QuestDest
    {
        uint32 map;
        float x, y, z;
        uint32 entry;
    };

    // Per-bot quest state (one active quest at a time, AC-equivalent).
    struct BotQuestState
    {
        uint32 questId;        // 0 = none
        uint32 destMap;
        float destX, destY, destZ;
        uint8  poiType;        // 0 = none, 1 = objective, 2 = taker
        time_t stateTs;        // state (re)started
        time_t reachTs;        // reached current POI (no-progress clock)
        uint32 progressBaseline;  // kill-objective progress at reach
        std::set<uint32> abandoned; // low-priority (no-progress) quests

        BotQuestState() : questId(0), destMap(0), destX(0), destY(0), destZ(0),
                          poiType(0), stateTs(0), reachTs(0), progressBaseline(0) {}
    };

    // One-time SQL load (call from PlayerBotMgr::Load at startup).
    static void Load();
    static bool IsLoaded() { return s_loaded; }
    static uint32 GetTakerCount() { return (uint32)s_takers.size(); }
    static uint32 GetObjectiveQuestCount() { return (uint32)s_objectives.size(); }
    static uint32 GetGiverEntryCount() { return (uint32)s_giverEntries.size(); }

    // Per-bot decision step (30s cadence from PlayerBotMgr::Update).
    // Full pipeline: validate state → COMPLETE→taker/turn-in →
    // INCOMPLETE→objective POI/no-progress → idle→log organize + accept.
    static void ProcessBot(Player* bot);

    // True while the bot has a quest to chase (travel/relocation skips).
    static bool HasActiveQuest(uint32 guidCounter);

    // Interaction primitives (packet replay through the bot's session).
    static bool AcceptQuest(Player* bot, ObjectGuid const& giverGuid, uint32 questId);
    static bool TurnInQuest(Player* bot, ObjectGuid const& takerGuid, uint32 questId);
    static bool DropQuest(Player* bot, uint32 questId);

private:
    // Decision helpers
    static bool WorthAccepting(Player const* bot, Quest const* quest);
    static void OrganizeQuestLog(Player* bot, BotQuestState& st);
    static uint32 KillProgress(Player* bot, Quest const* quest);
    static bool PickObjectivePoi(Player* bot, uint32 questId, BotQuestState& st);
    static bool PickTakerDest(Player* bot, uint32 questId, BotQuestState& st);
    static void TeleportToPoi(Player* bot, BotQuestState const& st);
    static Creature* FindLiveNpcNear(Player* bot, uint32 entry, float range);

    static bool s_loaded;
    static std::map<uint32 /*quest*/, std::vector<QuestDest> > s_takers;
    static std::map<uint32 /*quest*/, std::map<uint8 /*objIdx*/, std::vector<QuestDest> > > s_objectives;
    static std::set<uint32> s_giverEntries;
    static std::map<uint32 /*guid counter*/, BotQuestState> s_states;
};

#define sPlayerQuestMgr PlayerQuestMgr::getInstance()

#endif
