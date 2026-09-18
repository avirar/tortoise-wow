/*
 * Playerbot movement actions.
 *
 * Base MovementAction ported from AC mod-playerbots
 * (azerothcore-wotlk modules/mod-playerbots src/Ai/Base/Actions/MovementActions.h/.cpp,
 * 2939-line MovementAction base). Vanilla 1.18.1 adaptations (bot-master-plan.md R7):
 * - AC core PathGenerator -> this core's PathFinder/PathInfo (Maps/PathFinder.h,
 *   same PATHFIND_* family, calculate()/getPathType()/getActualEndPosition()).
 * - AC mm->MovePoint(x,y,z, generatePath) -> this core's
 *   MotionMaster::MovePoint(0,x,y,z, MOVE_PATHFINDING).
 * - AC GetMapHeight -> sMapMgr.GetHeight (INVALID_HEIGHT guard).
 * - WotLK bits dropped: vehicles (no Unit::GetVehicle in this core), backwards
 *   movement (no MovePointBackwards), PhaseMask/CollisionHeight water check
 *   (replaced by GetWaterOrGroundLevel compare), GetLiquidData swim-state
 *   management (this core manages swim flags in the spline layer).
 * - NewRpgBaseAction far-movement helpers (MoveFarTo / MoveWorldObjectTo /
 *   MoveRandomNear) live here as MovementAction protected methods: AC's
 *   NewRpgBaseAction is the consumer; our RPG actions (L5) consume the same
 *   methods. Stuck tracking for MoveFarTo lives in PlayerRpgInfo (AC rpgInfo).
 */

#ifndef _PLAYERBOT_MOVEMENT_ACTIONS_H
#define _PLAYERBOT_MOVEMENT_ACTIONS_H

#include "Action.h"
#include "Timer.h"
#include "ObjectGuid.h"
#include "LastMovementValue.h"
#include "MoveSplineInitArgs.h"  // Movement::PointsArray

class PlayerBotAI;
class Unit;
class WorldObject;

class MovementAction : public Action
{
public:
    MovementAction(PlayerBotAI* botAI, std::string const& name = "movement");
    virtual ~MovementAction() {}

    virtual bool Execute(Event event) override;
    virtual std::string const GetHint() { return "move"; }
    virtual std::string const GetEvent() { return "move"; }

    virtual bool isUseful() { return true; }
    virtual bool isPossible() { return true; }

protected:
    // --- existing chassis API (kept) ---
    bool Follow(Unit* target);
    bool MoveTo(float x, float y, float z);
    bool IsDuplicateMove(float x, float y, float z);
    bool IsMovingAllowed();
    void ClearIdleState();

    uint32 lastMoveTime;
    float lastMoveX;
    float lastMoveY;
    float lastMoveZ;

    // --- AC MovementAction port (MovementActions.cpp) ---
    // AC: MoveTo(mapId, x, y, z, idle, react, normal_only, exact_waypoint,
    //     priority, lessDelay, backwards)
    bool MoveTo(uint32 mapId, float x, float y, float z, bool idle = false, bool react = false,
                bool normal_only = false, bool exact_waypoint = false,
                MovementPriority priority = MovementPriority::MOVEMENT_NORMAL,
                bool lessDelay = false, bool backwards = false);
    bool MoveNear(uint32 mapId, float x, float y, float z, float distance = 0.0f,
                  MovementPriority priority = MovementPriority::MOVEMENT_NORMAL);
    float MoveDelay(float distance, bool backwards = false);
    void WaitForReach(float distance);
    void SetNextMovementDelay(float delayMillis);
    bool IsWaitingForLastMove(MovementPriority priority);
    void UpdateMovementState();
    const Movement::PointsArray SearchForBestPath(float x, float y, float z, float& modified_z,
                                                  int maxSearchCount = 3, bool normal_only = false, float step = 8.0f);
    void DoMovePoint(Unit* unit, float x, float y, float z, bool generatePath);

    // --- AC NewRpgBaseAction far-movement port (NewRpgBaseAction.cpp:40-300) ---
    // Long-distance walking: pathfinder routing + cone sampling + stuck->teleport
    // recovery. AC algorithm verbatim (PathGenerator->PathFinder, WorldPosition->map+x,y,z).
    bool MoveFarTo(uint32 mapId, float x, float y, float z);
    // Walk up to a world object (quest giver/taker) within `distance` of its position.
    bool MoveWorldObjectTo(ObjectGuid guid, float distance = INTERACTION_DISTANCE);
    // Random short-distance wander (multi-attempt, path-validated; AC moveStep=50).
    bool MoveRandomNear(float moveStep = 50.0f, MovementPriority priority = MovementPriority::MOVEMENT_NORMAL,
                        float cx = -1.0f, float cy = -1.0f, float cz = -1.0f);
};

class MoveRandomAction : public MovementAction
{
public:
    MoveRandomAction(PlayerBotAI* botAI) : MovementAction(botAI, "move random") {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

#endif
