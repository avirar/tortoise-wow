/*
 * Faithful port of AC mod-playerbots NewRpgInfo (NewRpgInfo.cpp).
 * See PlayerRpgInfo.h for the vanilla-reduction notes.
 */

#include "PlayerRpgInfo.h"
#include "Timer.h"

PlayerRpgInfo::PlayerRpgInfo() :
    status(RPG_IDLE),
    grindX(0.0f), grindY(0.0f), grindZ(0.0f),
    questId(0), objectiveIdx(-1),
    poiMapId(0), poiX(0.0f), poiY(0.0f), poiZ(0.0f), lastReachPOI(0),
    nearestMoveFarDis(FLT_MAX),
    stuckTs(0), stuckAttempts(0),
    moveFarMapId(0), moveFarX(0.0f), moveFarY(0.0f), moveFarZ(0.0f),
    startT(0), lastQuestInteract(0)
{
}

bool PlayerRpgInfo::HasStatusPersisted(uint32 maxDuration) const
{
    return (getMSTime() - startT) > maxDuration;
}

void PlayerRpgInfo::ChangeToGoGrind(float x, float y, float z)
{
    startT = getMSTime();
    status = RPG_GO_GRIND;
    grindX = x;
    grindY = y;
    grindZ = z;
}

void PlayerRpgInfo::ChangeToWanderRandom()
{
    startT = getMSTime();
    status = RPG_WANDER_RANDOM;
}

void PlayerRpgInfo::ChangeToDoQuest(uint32 qId, int32 objectiveIdx_)
{
    startT = getMSTime();
    status = RPG_DO_QUEST;
    questId = qId;
    objectiveIdx = objectiveIdx_;
    poiMapId = 0;
    poiX = poiY = poiZ = 0.0f;
    lastReachPOI = 0;
}

void PlayerRpgInfo::ChangeToRest()
{
    startT = getMSTime();
    status = RPG_REST;
}

void PlayerRpgInfo::ChangeToIdle()
{
    startT = getMSTime();
    status = RPG_IDLE;
}

void PlayerRpgInfo::Reset()
{
    status = RPG_IDLE;
    startT = getMSTime();
    lastQuestInteract = 0;
}

void PlayerRpgInfo::SetMoveFarTo(uint32 mapId, float x, float y, float z)
{
    nearestMoveFarDis = FLT_MAX;
    stuckTs = 0;
    stuckAttempts = 0;
    moveFarMapId = mapId;
    moveFarX = x;
    moveFarY = y;
    moveFarZ = z;
}

const char* PlayerRpgInfo::StatusToString(PlayerRpgStatus status)
{
    switch (status)
    {
        case RPG_IDLE:          return "idle";
        case RPG_GO_GRIND:      return "go_grind";
        case RPG_WANDER_RANDOM: return "wander_random";
        case RPG_DO_QUEST:      return "do_quest";
        case RPG_REST:          return "rest";
        default:                return "unknown";
    }
}
