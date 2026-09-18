/*
 * Faithful port of AC mod-playerbots NewRpgInfo
 * (azerothcore-wotlk modules/mod-playerbots src/Ai/World/Rpg/NewRpgInfo.h/.cpp).
 *
 * Vanilla 1.18.1 reductions (documented in bot-master-plan.md R7):
 * - WotLK statuses removed (RPG_GO_CAMP, RPG_WANDER_NPC, RPG_TRAVEL_FLIGHT,
 *   RPG_OUTDOOR_PVP are WotLK content); vanilla subset keeps AC's numeric
 *   values: RPG_IDLE=0, RPG_GO_GRIND=1, RPG_WANDER_RANDOM=3, RPG_DO_QUEST=5,
 *   RPG_REST=7.
 * - std::variant<...> data union -> explicit status enum + per-status fields
 *   (identical semantics: one active status, ChangeTo* resets startT).
 * - DoQuest.quest (Quest const*) dropped: this core caches QuestTemplate const*
 *   via sObjectMgr.GetQuestTemplate and it is looked up at decision time.
 */

#ifndef _PLAYERBOT_RPG_INFO_H
#define _PLAYERBOT_RPG_INFO_H

#include "Timer.h"
#include <cfloat>

// AC NewRpgStatus (PlayerbotAIConfig.h:57), vanilla subset.
enum PlayerRpgStatus : int
{
    RPG_IDLE = 0,
    RPG_GO_GRIND = 1,
    RPG_WANDER_RANDOM = 3,
    RPG_DO_QUEST = 5,
    RPG_REST = 7,
    // Array bound for per-status weight/duration tables (R7 L2). Not a real
    // status — kept at 8 (vanilla subset max + 1) so the indices 0..7 are valid.
    RPG_MAX_STATUS = 8
};

class PlayerRpgInfo
{
public:
    PlayerRpgInfo();

    PlayerRpgStatus GetStatus() const { return status; }
    bool HasStatusPersisted(uint32 maxDuration) const;
    void ChangeToGoGrind(float x, float y, float z);
    void ChangeToWanderRandom();
    void ChangeToDoQuest(uint32 questId, int32 objectiveIdx);
    void ChangeToRest();
    void ChangeToIdle();
    bool CanChangeTo(PlayerRpgStatus /*status*/) { return true; }  // AC: always true
    void Reset();
    void SetMoveFarTo(uint32 mapId, float x, float y, float z);
    static const char* StatusToString(PlayerRpgStatus status);

    // RPG_GO_GRIND
    float grindX;
    float grindY;
    float grindZ;

    // RPG_DO_QUEST (AC DoQuest: questId, objectiveIdx, pos, lastReachPOI)
    uint32 questId;
    int32 objectiveIdx;
    uint32 poiMapId;
    float poiX;
    float poiY;
    float poiZ;
    uint32 lastReachPOI;

    // MOVE_FAR (AC MOVE_FAR block: nearestMoveFarDis, stuckTs, stuckAttempts, moveFarPos)
    float nearestMoveFarDis;
    uint32 stuckTs;
    uint32 stuckAttempts;
    uint32 moveFarMapId;
    float moveFarX;
    float moveFarY;
    float moveFarZ;

    uint32 startT;  // start timestamp of the current status
uint32 lastQuestInteract;  // ms timestamp of the last quest accept/turn-in interaction (AC ForceToWait pacing)
private:
    PlayerRpgStatus status;
};

#endif
