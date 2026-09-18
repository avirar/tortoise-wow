/*
 * LastMovement implementations (AC mod-playerbots LastMovementValue.cpp port).
 */

#include "LastMovementValue.h"

#include "Timer.h"
#include "Unit.h"

LastMovement::LastMovement() :
    taxiMaster(),
    lastFollow(nullptr),
    lastAreaTrigger(0),
    lastFlee(0),
    lastMoveToMapId(0),
    lastMoveToX(0.0f),
    lastMoveToY(0.0f),
    lastMoveToZ(0.0f),
    lastMoveToOri(0.0f),
    lastdelayTime(0.0f),
    lastMoveShortX(0.0f),
    lastMoveShortY(0.0f),
    lastMoveShortZ(0.0f),
    msTime(0),
    priority(MovementPriority::MOVEMENT_NORMAL),
    nextTeleport(0)
{
}

void LastMovement::clear()
{
    taxiNodes.clear();
    taxiMaster = ObjectGuid();
    lastFollow = nullptr;
    lastAreaTrigger = 0;
    lastFlee = 0;
    lastMoveToMapId = 0;
    lastMoveToX = lastMoveToY = lastMoveToZ = lastMoveToOri = 0.0f;
    lastdelayTime = 0.0f;
    lastMoveShortX = lastMoveShortY = lastMoveShortZ = 0.0f;
    msTime = 0;
    priority = MovementPriority::MOVEMENT_NORMAL;
    nextTeleport = 0;
}

void LastMovement::Set(uint32 mapId, float x, float y, float z, float ori, float delayTime,
                       MovementPriority priority_)
{
    lastMoveToMapId = mapId;
    lastMoveToX = x;
    lastMoveToY = y;
    lastMoveToZ = z;
    lastMoveToOri = ori;
    lastFollow = nullptr;
    // AC: lastMoveShort = WorldPosition(mapId, x, y, z, ori) — IsDuplicateMove
    // compares against it while the delay window is open.
    lastMoveShortX = x;
    lastMoveShortY = y;
    lastMoveShortZ = z;
    msTime = getMSTime();
    lastdelayTime = delayTime;
    priority = priority_;
}

void LastMovement::Set(Unit* follow)
{
    Set(0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    lastMoveShortX = lastMoveShortY = lastMoveShortZ = 0.0f;
    lastFollow = follow;
    priority = MovementPriority::MOVEMENT_COMBAT;
}

void LastMovement::setShort(float x, float y, float z)
{
    lastMoveShortX = x;
    lastMoveShortY = y;
    lastMoveShortZ = z;
    lastFollow = nullptr;
    msTime = getMSTime();
}
