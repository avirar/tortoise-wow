#ifndef _PLAYERBOT_LASTMOVEMENTVALUE_H
#define _PLAYERBOT_LASTMOVEMENTVALUE_H

#include "Value.h"
#include "Timer.h"

class PlayerBotAI;
class Unit;

// High priority movement can override the previous low priority one
enum class MovementPriority
{
    MOVEMENT_IDLE,
    MOVEMENT_WANDER,
    MOVEMENT_NORMAL,
    MOVEMENT_COMBAT,
    MOVEMENT_FORCED
};

class LastMovement
{
public:
    LastMovement() { clear(); }

    void clear()
    {
        lastMoveToMapId = 0;
        lastMoveToX = 0;
        lastMoveToY = 0;
        lastMoveToZ = 0;
        lastMoveToOri = 0;
        lastFollow = nullptr;
        msTime = 0;
        priority = MovementPriority::MOVEMENT_NORMAL;
    }

    void Set(Unit* follow)
    {
        Set(0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        lastFollow = follow;
    }

    void Set(uint32 mapId, float x, float y, float z, float ori, float delayTime, MovementPriority pri = MovementPriority::MOVEMENT_NORMAL)
    {
        lastMoveToMapId = mapId;
        lastMoveToX = x;
        lastMoveToY = y;
        lastMoveToZ = z;
        lastMoveToOri = ori;
        lastFollow = nullptr;
        msTime = getMSTime();
        priority = pri;
    }

    uint32 lastMoveToMapId;
    float lastMoveToX;
    float lastMoveToY;
    float lastMoveToZ;
    float lastMoveToOri;
    Unit* lastFollow;
    uint32 msTime;
    MovementPriority priority;
};

class LastMovementValue : public ManualSetValue<LastMovement&>
{
public:
    LastMovementValue(PlayerBotAI* botAI) : ManualSetValue<LastMovement&>(botAI, data) {}

private:
    LastMovement data;
};

#endif
