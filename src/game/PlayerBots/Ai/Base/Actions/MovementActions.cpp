/*
 * Playerbot movement actions.
 *
 * Faithful port of AC mod-playerbots:
 * - src/Ai/Base/Actions/MovementActions.cpp (MovementAction base: MoveTo,
 *   MoveNear, MoveDelay, WaitForReach, SetNextMovementDelay, IsWaitingForLastMove,
 *   IsDuplicateMove, IsMovingAllowed, UpdateMovementState, SearchForBestPath,
 *   DoMovePoint)
 * - src/Ai/World/Rpg/Action/NewRpgBaseAction.cpp (MoveFarTo :40-204,
 *   MoveWorldObjectTo :205-238, MoveRandomNear :240+)
 *
 * Vanilla 1.18.1 adaptations (bot-master-plan.md R7, MovementActions.h notes):
 * PathGenerator->PathFinder, MovePoint(generatePath)->MovePoint(MOVE_PATHFINDING),
 * GetMapHeight->Map::GetHeight / sMapMgr, no vehicles/backwards/PhaseMask,
 * CanMove lives on PlayerBotAI (AC PlayerbotAI.cpp:6006).
 */

#include "MovementActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Object.h"
#include "Creature.h"
#include "GameObject.h"
#include "Group.h"
#include "Spell.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"
#include "PlayerRpgInfo.h"
#include "Logging.h"
#include "MotionMaster.h"
#include "Map.h"
#include "Maps/MapManager.h"
#include "GridMap.h"
#include "PathFinder.h"
#include "SharedDefines.h"
#include "Util.h"
#include "AiObjectContext.h"
#include <cmath>
#include <cstdlib>

// AC rand_norm_f() (azerothcore src/common/Utilities/Random.cpp) == this core's
// rand_norm_f() (Util.h) — uniform [0,1) float.

// 3D distance between two arbitrary points (AC GetExactDist equivalent for
// point pairs; Object::GetDistance* measures self-to-point only).
static float Dist3d(float x1, float y1, float z1, float x2, float y2, float z2)
{
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
}

MovementAction::MovementAction(PlayerBotAI* botAI, std::string const& name)
    : Action(botAI, name),
      lastMoveTime(0),
      lastMoveX(0),
      lastMoveY(0),
      lastMoveZ(0)
{
}

bool MovementAction::Execute([[maybe_unused]] Event event)
{
    return false;
}

bool MovementAction::Follow(Unit* target)
{
    if (!target || !target->IsAlive())
        return false;

    if (!IsMovingAllowed())
        return false;

    float dist = sServerFacade.GetDistance2d(bot, target);

    if (sServerFacade.IsDistanceGreaterThan(dist, sPlayerbotAIConfig.meleeDistance))
    {
        float x, y, z;
        target->GetClosePoint(x, y, z, 1.0f, sPlayerbotAIConfig.followDistance, 0, bot);
        return MoveTo(x, y, z);
    }

    return true;
}

bool MovementAction::MoveTo(float x, float y, float z)
{
    if (IsDuplicateMove(x, y, z))
        return false;

    if (!IsMovingAllowed())
        return false;

    bot->GetMotionMaster()->MovePoint(0, x, y, z, MOVE_PATHFINDING);
    lastMoveTime = getMSTime();
    lastMoveX = x;
    lastMoveY = y;
    lastMoveZ = z;

    return true;
}

// AC MovementAction::IsDuplicateMove (MovementActions.cpp:888): a move is a
// duplicate only while inside the last-move delay window AND the destination
// equals the last short move target.
bool MovementAction::IsDuplicateMove(float x, float y, float z)
{
    LastMovement& lastMove = AI_VALUE(LastMovement&, "last movement");

    // heuristic 5s
    if (lastMove.msTime + sPlayerbotAIConfig.maxWaitForMove < getMSTime() ||
        Dist3d(lastMove.lastMoveShortX, lastMove.lastMoveShortY, lastMove.lastMoveShortZ, x, y, z) > 0.01f)
        return false;

    return true;
}

// AC IsWaitingForLastMove (MovementActions.cpp:900): higher-priority movement
// overrides lower-priority pending waits; lower/equal waits while the
// last-move delay window is open.
bool MovementAction::IsWaitingForLastMove(MovementPriority priority)
{
    LastMovement& lastMove = AI_VALUE(LastMovement&, "last movement");

    if (priority > lastMove.priority)
        return false;

    // heuristic 5s
    if (lastMove.lastdelayTime + lastMove.msTime > getMSTime())
        return true;

    return false;
}

// AC IsMovingAllowed = botAI->CanMove() (PlayerbotAI.cpp:6006) + the existing
// chassis channel-cast check (stricter than AC; kept).
bool MovementAction::IsMovingAllowed()
{
    if (!botAI->CanMove())
        return false;

    if (bot->IsNonMeleeSpellCasted(false, false, false))
    {
        if (Spell* currentSpell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        {
            if (!(currentSpell->m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_MOVEMENT))
                return false;
        }

        if (Spell* currentSpell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            if (!(currentSpell->m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_MOVEMENT))
                return false;
        }
    }

    return true;
}

void MovementAction::ClearIdleState()
{
    lastMoveTime = 0;
    lastMoveX = 0;
    lastMoveY = 0;
    lastMoveZ = 0;
}

// AC UpdateMovementState (MovementActions.cpp:921), vanilla-reduced: no
// GetLiquidData swim/waterwalk state machine (this core manages the swim flag
// in the spline layer; waterwalk flag is set via .waterwalking only).
// Kept as the gate hook for the AC call sequence in MoveTo.
void MovementAction::UpdateMovementState()
{
    // no-op in this core (see header notes)
}

// AC MoveDelay (MovementActions.cpp:1290): distance / current mode speed.
float MovementAction::MoveDelay(float distance, bool backwards)
{
    float speed;
    if (bot->IsSwimming())
    {
        speed = backwards ? bot->GetSpeed(MOVE_SWIM_BACK) : bot->GetSpeed(MOVE_SWIM);
    }
    else if (bot->IsFlying())
    {
        // AC: MOVE_FLIGHT(_BACK); vanilla 1.18.1 has no flight rate slot —
        // flight speed rides the run rate in this core.
        speed = backwards ? bot->GetSpeed(MOVE_RUN_BACK) : bot->GetSpeed(MOVE_RUN);
    }
    else
    {
        speed = backwards ? bot->GetSpeed(MOVE_RUN_BACK) : bot->GetSpeed(MOVE_RUN);
    }
    if (speed <= 0.0f)
        speed = 0.5f;
    return distance / speed;
}

// AC WaitForReach (MovementActions.cpp:1310): stop the AI for the estimated
// time to reach the point (capped by maxWaitForMove, by globalCoolDown when
// a target is engaged).
void MovementAction::WaitForReach(float distance)
{
    float delay = 1000.0f * MoveDelay(distance);

    if (delay > sPlayerbotAIConfig.maxWaitForMove)
        delay = sPlayerbotAIConfig.maxWaitForMove;

    Unit* target = AI_VALUE(Unit*, "current target");
    if (target && delay > sPlayerbotAIConfig.globalCoolDown)
        delay = sPlayerbotAIConfig.globalCoolDown;

    if (delay < 0)
        delay = 0;

    botAI->SetNextCheckDelay((uint32)delay);
}

// AC SetNextMovementDelay (MovementActions.cpp:1329): like SetNextCheckDelay
// but only blocks movement (MOVEMENT_FORCED priority).
void MovementAction::SetNextMovementDelay(float delayMillis)
{
    AI_VALUE(LastMovement&, "last movement")
        .Set(bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetOrientation(),
             delayMillis, MovementPriority::MOVEMENT_FORCED);
}

// AC MoveTo (MovementActions.cpp:169): the main walk entry point.
// generatePath off while flying/swimming (spline straight line); otherwise
// validate the destination via SearchForBestPath (z modification) and issue
// MovePoint with pathfinding. LastMovement records the commit + delay.
bool MovementAction::MoveTo(uint32 mapId, float x, float y, float z, bool /*idle*/, bool /*react*/,
                            bool normal_only, bool exact_waypoint, MovementPriority priority,
                            bool lessDelay, bool /*backwards*/)
{
    UpdateMovementState();
    if (!IsMovingAllowed())
    {
        return false;
    }
    if (IsDuplicateMove(x, y, z))
    {
        return false;
    }
    if (IsWaitingForLastMove(priority))
    {
        return false;
    }

    if (mapId != bot->GetMapId())
    {
        // Cross-map movement is the caller's job (MoveFarTo handles the
        // stuck-recovery teleport; quest POI selection is same-map by design,
        // AC NewRPG GetQuestPOIPosAndObjectiveIdx).
        return false;
    }

    bool generatePath = !bot->IsFlying() && !bot->IsSwimming();
    if (exact_waypoint || !generatePath)
    {
        float distance = bot->GetDistance3dToCenter(x, y, z);
        if (distance > 0.01f)
        {
            if (bot->GetStandState() != UNIT_STAND_STATE_STAND)
                bot->SetStandState(UNIT_STAND_STATE_STAND);

            DoMovePoint(bot, x, y, z, generatePath);
            float delay = 1000.0f * MoveDelay(distance, false);
            if (lessDelay)
            {
                delay -= (float)sPlayerbotAIConfig.reactDelay;
            }
            delay = std::max(0.0f, delay);
            delay = std::min((float)sPlayerbotAIConfig.maxWaitForMove, delay);
            AI_VALUE(LastMovement&, "last movement").Set(mapId, x, y, z, bot->GetOrientation(), delay, priority);
            return true;
        }
        return false;
    }

    float modifiedZ;
    SearchForBestPath(x, y, z, modifiedZ, sPlayerbotAIConfig.maxMovementSearchTime, normal_only);
    if (modifiedZ <= INVALID_HEIGHT)
        return false;
    float distance = bot->GetDistance3dToCenter(x, y, modifiedZ);
    if (distance > 0.01f)
    {
        if (bot->GetStandState() != UNIT_STAND_STATE_STAND)
            bot->SetStandState(UNIT_STAND_STATE_STAND);

        DoMovePoint(bot, x, y, modifiedZ, generatePath);
        float delay = 1000.0f * MoveDelay(distance, false);
        if (lessDelay)
        {
            delay -= (float)sPlayerbotAIConfig.reactDelay;
        }
        delay = std::max(0.0f, delay);
        delay = std::min((float)sPlayerbotAIConfig.maxWaitForMove, delay);
        AI_VALUE(LastMovement&, "last movement")
            .Set(mapId, x, y, modifiedZ, bot->GetOrientation(), delay, priority);
        return true;
    }

    return false;
}

// AC MoveNear (MovementActions.cpp:82): walk to a point `distance` from
// (x,y,z) at a random angle.
bool MovementAction::MoveNear(uint32 mapId, float x, float y, float z, float distance, MovementPriority priority)
{
    float angle = (float)rand_norm_f() * 2.0f * (float)M_PI;
    return MoveTo(mapId, x + cos(angle) * distance, y + sin(angle) * distance, z, false, false, false, false, priority);
}

// AC DoMovePoint (MovementActions.cpp:1768): clear + MovePoint.
// AC core: mm->MovePoint(0,x,y,z, FORCED_MOVEMENT_NONE, 0,0, generatePath);
// this core: MovePoint(0,x,y,z, MOVE_PATHFINDING).
void MovementAction::DoMovePoint(Unit* unit, float x, float y, float z, bool generatePath)
{
    if (!unit)
        return;

    MotionMaster* mm = unit->GetMotionMaster();
    if (!mm)
        return;

    mm->Clear();
    uint32 options = MOVE_NONE;
    if (generatePath)
        options |= MOVE_PATHFINDING;
    mm->MovePoint(0, x, y, z, options);
}

// AC SearchForBestPath (MovementActions.cpp:1699): pathfind to (x,y,z) with a
// z-modification search. AC PathGenerator -> this core's PathFinder (same
// PATHFIND_* family); GetMapHeight -> sMapMgr/Map height.
const Movement::PointsArray MovementAction::SearchForBestPath(float x, float y, float z, float& modified_z,
                                                              int maxSearchCount, bool normal_only, float step)
{
    bool found = false;
    modified_z = INVALID_HEIGHT;

    Map* map = bot->GetMap();
    float tempZ = map->GetHeight(x, y, z);
    if (tempZ <= INVALID_HEIGHT)
        return Movement::PointsArray();

    PathFinder gen(bot);
    gen.calculate(x, y, tempZ);
    Movement::PointsArray result = gen.getPath();
    float min_length = gen.Length();
    int typeOk = PATHFIND_NORMAL | PATHFIND_INCOMPLETE;
    if ((uint32)gen.getPathType() & typeOk && std::abs(tempZ - z) < 0.5f)
    {
        modified_z = tempZ;
        return result;
    }
    // Start searching
    if ((uint32)gen.getPathType() & typeOk)
    {
        modified_z = tempZ;
        found = true;
    }
    int count = 1;
    for (float delta = step; count < maxSearchCount / 2 + 1; count++, delta += step)
    {
        tempZ = map->GetHeight(x, y, z + delta);
        if (tempZ <= INVALID_HEIGHT)
        {
            continue;
        }
        PathFinder gen(bot);
        gen.calculate(x, y, tempZ);
        if ((uint32)gen.getPathType() & typeOk && gen.Length() < min_length)
        {
            found = true;
            min_length = gen.Length();
            result = gen.getPath();
            modified_z = tempZ;
        }
    }
    for (float delta = -step; count < maxSearchCount; count++, delta -= step)
    {
        tempZ = map->GetHeight(x, y, z + delta);
        if (tempZ <= INVALID_HEIGHT)
        {
            continue;
        }
        PathFinder gen(bot);
        gen.calculate(x, y, tempZ);
        if ((uint32)gen.getPathType() & typeOk && gen.Length() < min_length)
        {
            found = true;
            min_length = gen.Length();
            result = gen.getPath();
            modified_z = tempZ;
        }
    }
    if (!found && normal_only)
    {
        modified_z = INVALID_HEIGHT;
        return Movement::PointsArray();
    }
    if (!found && !normal_only)
    {
        return result;
    }
    return result;
}

// AC NewRpgBaseAction::MoveFarTo (NewRpgBaseAction.cpp:40-204) verbatim
// (PathGenerator->PathFinder; WorldPosition->(mapId,x,y,z);
// PATHFIND_FARFROMPOLY->PATHFIND_SHORTCUT — documented adaptation; this core's
// PathType has no FARFROMPOLY).
bool MovementAction::MoveFarTo(uint32 mapId, float x, float y, float z)
{
    // Don't start a ground move while the bot is on a taxi: the MovePoint
    // issued below would fight the active flight spline. Returning true
    // consumes the tick (AC comment, verbatim intent).
    if (bot->HasUnitState(UNIT_STAT_TAXI_FLIGHT))
        return true;

    // New dest -> clear stuck information (AC rpgInfo.SetMoveFarTo).
    if (mapId != botAI->rpgInfo.moveFarMapId ||
        std::abs(botAI->rpgInfo.moveFarX - x) > 0.01f ||
        std::abs(botAI->rpgInfo.moveFarY - y) > 0.01f ||
        std::abs(botAI->rpgInfo.moveFarZ - z) > 0.01f)
    {
        botAI->rpgInfo.SetMoveFarTo(mapId, x, y, z);
    }

    // performance optimization
    if (IsWaitingForLastMove(MovementPriority::MOVEMENT_NORMAL))
    {
        return false;
    }

    // Let previously committed movement finish before recomputing
    // (AC committed-movement early-out; prevents mid-walk re-pathing
    // oscillation around unreachable destinations).
    {
        LastMovement& lastMove = AI_VALUE(LastMovement&, "last movement");
        if (bot->IsMoving() && lastMove.lastMoveToMapId == bot->GetMapId())
        {
            float remaining = Dist3d(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
                                     lastMove.lastMoveToX, lastMove.lastMoveToY, lastMove.lastMoveToZ);
            if (remaining > 10.0f)
                return true;
        }
    }

    // stuck check
    float disToDest = bot->GetDistance3dToCenter(x, y, z);
    // Require a meaningful improvement (5yd) to reset the stuck counter.
    if (disToDest + 5.0f < botAI->rpgInfo.nearestMoveFarDis)
    {
        botAI->rpgInfo.nearestMoveFarDis = disToDest;
        botAI->rpgInfo.stuckTs = getMSTime();
        botAI->rpgInfo.stuckAttempts = 0;
    }
    else if (++botAI->rpgInfo.stuckAttempts >= 5 &&
             (getMSTime() - botAI->rpgInfo.stuckTs) >= sPlayerbotAIConfig.moveStuckTime)
    {
        // No meaningful progress toward dest for moveStuckTime: fall back to
        // teleporting directly (AC stuck-recovery; the ONLY sanctioned
        // teleport in the quest/RPG flow).
        botAI->rpgInfo.stuckTs = getMSTime();
        botAI->rpgInfo.stuckAttempts = 0;
        LOG_DEBUG("playerbots", "playerbots: %s stuck moving far from (%.0f,%.0f,%.0f m%u) to (%.0f,%.0f,%.0f m%u) - teleporting",
                  bot->GetName(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetMapId(),
                  x, y, z, mapId);
        bot->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED | AURA_INTERRUPT_FLAG_CHANGE_MAP);
        bot->TeleportTo(mapId, x, y, z, 0.0f);
        return true;
    }

    float dis = bot->GetDistance3dToCenter(x, y, z);
    if (dis < sPlayerbotAIConfig.pathFinderDis)
    {
        return MoveTo(mapId, x, y, z, false, false, false, true);
    }

    // Primary strategy: ask the pathfinder for a route to the TRUE destination.
    // PATHFIND_SHORTCUT stands in for AC's PATHFIND_FARFROMPOLY (documented).
    const uint32 typeOk = PATHFIND_NORMAL | PATHFIND_INCOMPLETE | PATHFIND_SHORTCUT;

    {
        PathFinder path(bot);
        path.calculate(x, y, z);
        PathType type = path.getPathType();
        bool canReach = !((uint32)type & (~typeOk));
        if (canReach)
        {
            float endX, endY, endZ;
            path.getActualEndPosition(endX, endY, endZ);
            // Only commit if the endpoint actually makes progress toward the
            // destination (AC pathological-INCOMPLETE guard).
            float endDistToDest = Dist3d(x, y, z, endX, endY, endZ);
            if (endDistToDest + 5.0f < disToDest)
            {
                return MoveTo(mapId, endX, endY, endZ, false, false, false, true);
            }
        }
    }

    // Fallback: pathfinder couldn't route to the destination. Sample the
    // forward cone for a reachable stepping stone (AC cone sampling, 2
    // samples, cap kept for performance).
    float minDelta = M_PI;
    const float bx = bot->GetPositionX();
    const float by = bot->GetPositionY();
    const float bz = bot->GetPositionZ();
    float baseAngle = bot->GetAngle(x, y);
    float rx, ry, rz;
    bool found = false;
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        float delta = (rand_norm_f() - 0.5f) * (float)M_PI;  // ±π/2, forward cone
        float sampleDis = (0.5f + rand_norm_f() * 0.5f) * sPlayerbotAIConfig.pathFinderDis;
        float angle = baseAngle + delta;
        float dx = bx + cos(angle) * sampleDis;
        float dy = by + sin(angle) * sampleDis;
        float dz = bz + 0.5f;
        PathFinder path(bot);
        path.calculate(dx, dy, dz);
        PathType type = path.getPathType();
        bool canReach = !((uint32)type & (~typeOk));

        if (canReach && std::abs(delta) <= minDelta)
        {
            found = true;
            float endX, endY, endZ;
            path.getActualEndPosition(endX, endY, endZ);
            rx = endX;
            ry = endY;
            rz = endZ;
            minDelta = std::abs(delta);
        }
    }
    if (found)
    {
        return MoveTo(mapId, rx, ry, rz, false, false, false, true);
    }
    return false;
}

// AC NewRpgBaseAction::MoveWorldObjectTo (NewRpgBaseAction.cpp:205-238):
// walk up to a world object (quest giver/taker) within `distance` of its
// position, at a jittered approach angle.
bool MovementAction::MoveWorldObjectTo(ObjectGuid guid, float distance)
{
    if (IsWaitingForLastMove(MovementPriority::MOVEMENT_NORMAL))
    {
        return false;
    }

    WorldObject* object = bot->GetMap()->GetWorldObject(guid);
    if (!object)
        return false;
    float x = object->GetPositionX();
    float y = object->GetPositionY();
    float z = object->GetPositionZ();
    float mapId = object->GetMapId();
    float angle = 0.f;

    if (!object->ToUnit() || !object->ToUnit()->IsMoving())
        angle = object->GetAngle(bot) + ((float)M_PI * irand(-25, 25) / 100.0f);  // closest approach w/ jitter
    else
        angle = object->GetOrientation() + ((float)M_PI * irand(-25, 25) / 100.0f);  // lead its movement

    float rnd = rand_norm_f();
    x += cos(angle) * distance * rnd;
    y += sin(angle) * distance * rnd;
    if (!MapManager::IsValidMapCoord(mapId, x, y, z))
    {
        x = object->GetPositionX();
        y = object->GetPositionY();
        z = object->GetPositionZ();
    }
    return MoveTo(mapId, x, y, z, false, false, false, true);
}

// AC NewRpgBaseAction::MoveRandomNear (NewRpgBaseAction.cpp:240+): random
// short wander, multi-attempt with path + water validation (single-sample
// version was rejected upstream — bad rolls locked bots in place).
bool MovementAction::MoveRandomNear(float moveStep, MovementPriority priority,
                                    float cx, float cy, float cz)
{
    if (IsWaitingForLastMove(priority))
        return false;

    const float x = (cx < 0.0f) ? bot->GetPositionX() : cx;
    const float y = (cy < 0.0f) ? bot->GetPositionY() : cy;
    const float z = (cz < 0.0f) ? bot->GetPositionZ() : cz;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        float distance = (0.4f + rand_norm_f() * 0.6f) * moveStep;
        float angle = (float)rand_norm_f() * 2.0f * (float)M_PI;
        float dx = x + distance * cos(angle);
        float dy = y + distance * sin(angle);
        float dz = z;

        PathFinder path(bot);
        path.calculate(dx, dy, dz);
        PathType type = path.getPathType();
        uint32 typeOk = PATHFIND_NORMAL | PATHFIND_INCOMPLETE | PATHFIND_SHORTCUT;
        bool canReach = !((uint32)type & (~typeOk));

        if (!canReach)
            continue;

        if (!MapManager::IsValidMapCoord(bot->GetMapId(), dx, dy, dz))
            continue;

        // Don't aim underwater (AC IsInWater+PhaseMask check, vanilla-reduced:
        // water-level compare via terrain info).
        TerrainInfo const* terrain = bot->GetTerrain();
        if (terrain)
        {
            float waterLevel = terrain->GetWaterOrGroundLevel(dx, dy, dz);
            if (dz < waterLevel - 0.5f)
                continue;
        }

        bool moved = MoveTo(bot->GetMapId(), dx, dy, dz, false, false, false, true, priority);
        if (moved)
            return true;
    }

    return false;
}

bool MoveRandomAction::Execute(Event /*event*/)
{
    // AC wander = MoveRandomNear (path-validated); the old straight-line
    // random-walk sample is superseded by the faithful port.
    return MoveRandomNear(50.0f, MovementPriority::MOVEMENT_WANDER);
}

bool MoveRandomAction::isUseful()
{
    if (bot->IsInCombat())
        return false;

    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr)
        return false;

    if (bot->HasUnitState(UNIT_STAT_TAXI_FLIGHT))
        return false;

    // AC pattern: wander only for solo bots or group leaders
    // Group followers follow the leader, don't wander independently
    if (Group* group = bot->GetGroup())
    {
        if (group->GetLeaderGuid() != ObjectGuid(bot->GetGUID()))
            return false;  // not group leader, don't wander
    }

    return true;
}
