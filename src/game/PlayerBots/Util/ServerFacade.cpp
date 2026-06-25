#include "ServerFacade.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "Maps/GridNotifiers.h"
#include "Maps/MapManager.h"
#include "Maps/CellImpl.h"
#include "Logging.h"
#include "Log.h"

struct NearbyCreatureDebugger
{
    Unit const* me;
    float range;
    uint32 totalScanned;
    uint32 rejectedDistance;
    uint32 rejectedFriendly;
    uint32 rejectedDead;
    uint32 rejectedSelf;
    uint32 passed;
    uint32 maxLog;

    NearbyCreatureDebugger()
        : me(nullptr), range(0), totalScanned(0), rejectedDistance(0), rejectedFriendly(0),
          rejectedDead(0), rejectedSelf(0), passed(0), maxLog(5) {}

    void Init(Unit const* m, float r)
    {
        me = m;
        range = r;
        totalScanned = 0;
        rejectedDistance = 0;
        rejectedFriendly = 0;
        rejectedDead = 0;
        rejectedSelf = 0;
        passed = 0;
    }

    void CheckCreature(Creature* cre)
    {
        ++totalScanned;
        if (cre == me->ToUnit())
        { ++rejectedSelf; return; }
        if (!cre->IsAlive())
        { ++rejectedDead; return; }
        float d = me->GetDistance2d(cre);
        if (d > range)
        { ++rejectedDistance; return; }
        if (me->IsFriendlyTo(cre))
        { ++rejectedFriendly; return; }
        ++passed;
        if (passed <= maxLog)
        {
            LOG_DEBUG("playerbots", "  [DEBUG-SCAN] PASS #%u: entry=%u '%s' dist=%.1f reaction=%d alive=%d",
                passed, cre->GetEntry(), cre->GetName(), d,
                (int)me->GetReactionTo(cre), cre->IsAlive() ? 1 : 0);
        }
    }

    void CheckPlayer(Player* pl)
    {
        ++totalScanned;
        if (pl == me->ToPlayer())
        { ++rejectedSelf; return; }
        if (!pl->IsAlive())
        { ++rejectedDead; return; }
        float d = me->GetDistance2d(pl);
        if (d > range)
        { ++rejectedDistance; return; }
        if (me->IsFriendlyTo(pl))
        { ++rejectedFriendly; return; }
        ++passed;
        if (passed <= maxLog)
        {
            LOG_DEBUG("playerbots", "  [DEBUG-SCAN] PASS #%u: PLAYER '%s' dist=%.1f reaction=%d",
                passed, pl->GetName(), d, (int)me->GetReactionTo(pl));
        }
    }

    void Visit(PlayerMapType &m)
    {
        for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            CheckPlayer(itr->getSource());
    }

    void Visit(CreatureMapType &m)
    {
        for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            CheckCreature(itr->getSource());
    }

    template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    template<class NOT_INTERESTED> void Visit(NOT_INTERESTED &) {}
};

float ServerFacade::GetDistance2d(Unit* unit, WorldObject* wo)
{
    if (!unit || !wo)
        return 0.f;

    return unit->GetDistance2d(wo);
}

float ServerFacade::GetDistance2d(Unit* unit, float x, float y)
{
    if (!unit)
        return 0.f;

    return unit->GetDistance2d(x, y);
}

bool ServerFacade::IsDistanceLessThan(float dist1, float dist2)
{
    return dist1 < dist2;
}

bool ServerFacade::IsDistanceGreaterThan(float dist1, float dist2)
{
    return dist1 > dist2;
}

bool ServerFacade::IsDistanceGreaterOrEqualThan(float dist1, float dist2)
{
    return !IsDistanceLessThan(dist1, dist2);
}

bool ServerFacade::IsDistanceLessOrEqualThan(float dist1, float dist2)
{
    return !IsDistanceGreaterThan(dist1, dist2);
}

void ServerFacade::SetFacingTo(Player* bot, WorldObject* wo, bool /*force*/)
{
    if (!bot || !wo)
        return;

    float angle = bot->GetAngle(wo);
    bot->SetOrientation(angle);
}

Unit* ServerFacade::GetChaseTarget(Unit* target)
{
    if (!target)
        return nullptr;

    MovementGenerator* movementGen = target->GetMotionMaster()->top();
    if (movementGen && movementGen->GetMovementGeneratorType() == CHASE_MOTION_TYPE)
    {
        return target->GetVictim();
    }

    return nullptr;
}

void ServerFacade::SendPacket(Player* player, WorldPacket* packet)
{
    if (player && packet && player->GetSession())
        player->GetSession()->SendPacket(packet);
}

Unit* ServerFacade::SelectNearestHostileTarget(Unit* unit, float range)
{
    if (!unit || !unit->IsAlive())
        return nullptr;

    return unit->SelectNearestUnfriendlyTarget(range);
}

void ServerFacade::DebugNearbyCreatures(Unit* unit, float range, const char* caller)
{
    if (!unit || !unit->IsAlive())
        return;

    Map* map = unit->GetMap();
    if (!map || map->IsDungeon())
        return;

    NearbyCreatureDebugger debugger;
    debugger.Init(unit, range);

    CellPair p(MaNGOS::ComputeCellPair(unit->GetPositionX(), unit->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();

    TypeContainerVisitor<NearbyCreatureDebugger, WorldTypeMapContainer> world_vis(debugger);
    TypeContainerVisitor<NearbyCreatureDebugger, GridTypeMapContainer> grid_vis(debugger);

    cell.Visit(p, world_vis, *map, *unit, range);
    cell.Visit(p, grid_vis, *map, *unit, range);

    LOG_DEBUG("playerbots", "%s [%s] pos=(%.1f,%.1f,%.1f) map=%u range=%.0f | scanned=%u | PASS=%u | rej:dist=%u friendly=%u dead=%u self=%u",
        unit->GetName(), caller, unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(),
        map->GetId(), range,
        debugger.totalScanned, debugger.passed, debugger.rejectedDistance, debugger.rejectedFriendly, debugger.rejectedDead, debugger.rejectedSelf);
}
