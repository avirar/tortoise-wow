/*
 * Faithful port of AC mod-playerbots LastMovementValue
 * (azerothcore-wotlk modules/mod-playerbots src/Ai/Base/Value/LastMovementValue.h).
 *
 * Vanilla 1.18.1 reductions (documented in bot-master-plan.md R7):
 * - AC WorldPosition (mapId+x,y,z) -> plain x,y,z floats: short moves are same-map.
 * - WotLK walking-travel fields (TravelPath lastPath, std::future) removed;
 *   walking travel is the R6 TravelSystem port.
 * - taxiNodes/taxiMaster kept: taxi flight exists in this core (MoveTaxiFlight).
 */

#ifndef _PLAYERBOT_LAST_MOVEMENT_VALUE_H
#define _PLAYERBOT_LAST_MOVEMENT_VALUE_H

#include "Value.h"
#include "ObjectGuid.h"
#include <vector>

class Unit;

// AC pattern: higher-priority movement overrides lower-priority pending waits.
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
    LastMovement();

    void clear();
    void Set(uint32 mapId, float x, float y, float z, float ori, float delayTime,
             MovementPriority priority = MovementPriority::MOVEMENT_NORMAL);
    void Set(Unit* follow);
    void setShort(float x, float y, float z);

    std::vector<uint32> taxiNodes;   // reserved: taxi path (R6)
    ObjectGuid taxiMaster;
    Unit* lastFollow;
    uint32 lastAreaTrigger;
    time_t lastFlee;
    uint32 lastMoveToMapId;
    float lastMoveToX;
    float lastMoveToY;
    float lastMoveToZ;
    float lastMoveToOri;
    float lastdelayTime;
    float lastMoveShortX;
    float lastMoveShortY;
    float lastMoveShortZ;
    uint32 msTime;
    MovementPriority priority;
    time_t nextTeleport;
};

class LastMovementValue : public ManualSetValue<LastMovement&>
{
public:
    LastMovementValue(PlayerBotAI* botAI) : ManualSetValue<LastMovement&>(botAI, data) {}

private:
    LastMovement data;
};

class StayTimeValue : public ManualSetValue<time_t>
{
public:
    StayTimeValue(PlayerBotAI* botAI) : ManualSetValue<time_t>(botAI, 0) {}
};

#endif
