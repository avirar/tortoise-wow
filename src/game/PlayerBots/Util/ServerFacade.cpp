#include "ServerFacade.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "Maps/GridNotifiers.h"
#include "Maps/MapManager.h"
#include "Maps/Map.h"
#include "Maps/CellImpl.h"
#include "Group.h"
#include "PlayerBotAI.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "Logging.h"
#include "Log.h"

// Collects nearby unfriendly units for target selection
struct NearbyUnfriendlyCollector
{
    Unit const* me;
    float range;
    std::vector<Unit*> candidates;

    NearbyUnfriendlyCollector()
        : me(nullptr), range(0) {}

    void Init(Unit const* m, float r)
    {
        me = m;
        range = r;
        candidates.clear();
    }

    void Visit(PlayerMapType &m)
    {
        for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            Player* pl = itr->getSource();
            if (pl && pl != me->ToPlayer() && pl->IsAlive() && !me->IsFriendlyTo(pl) && me->GetDistance2d(pl) <= range)
                candidates.push_back(pl);
        }
    }

    void Visit(CreatureMapType &m)
    {
        for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            Creature* cre = itr->getSource();
            if (cre && cre != me->ToUnit() && cre->IsAlive() && !me->IsFriendlyTo(cre) && me->GetDistance2d(cre) <= range)
                candidates.push_back(cre);
        }
    }

    template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    template<class NOT_INTERESTED> void Visit(NOT_INTERESTED &) {}
};

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

// AC GrindTargetValue::GetTargetingPlayerCount() pattern
// Count how many group members (including bots) are already targeting this unit
uint32 ServerFacade::GetTargetingPlayerCount(Player* bot, Unit* target)
{
    if (!bot || !target)
        return 0;

    Group* group = bot->GetGroup();
    if (!group)
        return 0;

    uint32 count = 0;
    Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
    for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); ++itr)
    {
        Player* member = ObjectAccessor::FindPlayer(itr->guid);
        if (!member || !member->IsAlive() || member == bot)
            continue;

        // Check if this member is a bot with "current target" set to our target
        PlayerAI* memberAI = member->AI();
        PlayerBotAI* botAI = dynamic_cast<PlayerBotAI*>(memberAI);
        if (botAI)
        {
            AiObjectContext* ctx = botAI->GetAiObjectContext();
            if (ctx)
            {
                Unit* memberTarget = ctx->GetValue<Unit*>("current target")->Get();
                if (memberTarget == target)
                    ++count;
            }
        }
        else if (member->GetSelectionGuid() == ObjectGuid(target->GetGUID()))
        {
            // Real player targeting this unit
            ++count;
        }
    }

    return count;
}

Unit* ServerFacade::SelectNearestSafeTarget(Player* bot, float range)
{
    if (!bot || !bot->IsAlive())
        return nullptr;

    Map* map = bot->GetMap();
    if (!map || map->IsDungeon())
        return nullptr;

    NearbyUnfriendlyCollector collector;
    collector.Init(bot, range);

    CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();

    TypeContainerVisitor<NearbyUnfriendlyCollector, WorldTypeMapContainer> world_vis(collector);
    TypeContainerVisitor<NearbyUnfriendlyCollector, GridTypeMapContainer> grid_vis(collector);

    cell.Visit(p, world_vis, *map, *bot, range);
    cell.Visit(p, grid_vis, *map, *bot, range);

    // AC GrindTargetValue pattern: skip targets already being targeted by group members
    Unit* bestTarget = nullptr;
    float bestDist = range;
    uint32 filtered = 0;
    for (Unit* candidate : collector.candidates)
    {
        if (!candidate || !candidate->IsAlive() || bot->IsFriendlyTo(candidate))
        { ++filtered; continue; }

        // AC pattern: skip if another group member is already targeting this
        if (GetTargetingPlayerCount(bot, candidate) > 0)
        { ++filtered; continue; }

        // AC pattern: skip creatures that don't give XP (critters, trainers, etc.)
        if (!bot->IsHonorOrXPTarget(candidate))
        { ++filtered; continue; }

        // Skip creatures already being attacked by someone else
        if (Creature* c = candidate->ToCreature())
        {
            if (c->GetVictim() && c->GetVictim() != bot)
            { ++filtered; continue; }  // already attacking someone else
            // NOTE: tap check is for LOOT only, not for combat selection
        }

        float dist = GetDistance2d(bot, candidate);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestTarget = candidate;
        }
    }

    LOG_DEBUG("playerbots", "%s [SelectNearestSafeTarget] candidates=%u filtered=%u best=%s dist=%.1f",
        bot->GetName(), (uint32)collector.candidates.size(), filtered,
        bestTarget ? bestTarget->GetName() : "none", bestDist);

    return bestTarget;
}
