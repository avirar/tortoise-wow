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
#include "PlayerbotAIConfig.h"
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

// AC GrindTargetValue::GetTargetingPlayerCount() pattern — R4 group-assist
// primitive (currently unused by SelectNearestSafeTarget: tap-claim model
// replaced proximity/targeting heuristics; group loot + IsTappedBy handle
// grouped contention authoritatively). Reserved for player-invited bot
// groups: assist prioritization, role coordination, dungeon/raid support.
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

// R5d idle-relocation tracking: last time each bot had a viable grind target.
// Keyed by GUID; bounded by online-account count (pruned wholesale if it ever
// exceeds a sane ceiling — wipe cycles create new GUIDs over long uptimes).
static std::map<ObjectGuid, time_t> s_lastViableGrindTarget;

time_t ServerFacade::SecondsWithoutViableGrindTarget(Player* bot)
{
    if (!bot)
        return 0;
    if (s_lastViableGrindTarget.size() > 4096)
        s_lastViableGrindTarget.clear();               // safety valve
    ObjectGuid guid = bot->GetObjectGuid();
    std::map<ObjectGuid, time_t>::iterator i = s_lastViableGrindTarget.find(guid);
    if (i == s_lastViableGrindTarget.end())
    {
        s_lastViableGrindTarget[guid] = time(nullptr); // first seen: start grace clock
        return 0;
    }
    return time(nullptr) - i->second;
}

void ServerFacade::MarkViableGrindTargetSeen(Player* bot)
{
    if (bot)
        s_lastViableGrindTarget[bot->GetObjectGuid()] = time(nullptr);
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

    // AC GrindTargetValue pattern + AttackersValue tap-claim model:
    // A player CLAIMS a creature by damaging it (or landing a hostile spell)
    // — that is what sets its loot recipient (the "tap"). Nothing else is a
    // claim: proximity to other bots, or other bots targeting/walking toward
    // the creature, does NOT make it claimed (user-directed; the old
    // targeting-count contention penalty was removed as wrong-headed).
    //  - tapped by me            -> valid (finish my kill)
    //  - tapped by someone else  -> skip (their loot/XP, attacking is waste)
    //  - untapped, victim is another player's pull-in-progress -> skip
    //    (AC AttackersValue clause 2; they will tap within a swing)
    //  - untapped, everything else -> fair game: attack and tap it
    // R5c XP-primary scoring (reference-checked):
    //  - AC: hard filters (XP-eligible, level cap, non-elite, z-diff, LOS)
    //    then NEAREST wins — XP is a filter, not a score.
    //  - Shyalya/ike3: same + soft distance penalties for low-XP mobs.
    // Tortoise: effDist = dist + max(0, botLevel - mobLevel) * xpLevelPenalty
    // (below-level mobs look farther; at/above-level mobs are NOT given a
    // negative bonus — level above bot is capped by maxTargetLevelDiff,
    // never actively preferred: no death-risk amplification).
    Unit* best = nullptr;
    float bestEffDist = range * 2.0f + 100.0f;    // well above any real effDist
    uint32 filtered = 0;
    uint32 rFriendly = 0, rNoXP = 0, rLevel = 0, rElite = 0, rZdiff = 0, rTapped = 0, rVictim = 0, rLos = 0;
    for (Unit* candidate : collector.candidates)
    {
        if (!candidate || !candidate->IsAlive() || bot->IsFriendlyTo(candidate))
        { ++filtered; ++rFriendly; continue; }

        // AC pattern: skip creatures that don't give XP (critters, trainers, etc.)
        if (!bot->IsHonorOrXPTarget(candidate))
        { ++filtered; ++rNoXP; continue; }

        // AC pattern: skip targets without line-of-sight — otherwise the
        // attack action fails every tick (target behind a cliff/rock), the
        // bot is stuck in an action-FAILED loop forever.
        if (!bot->IsWithinLOS(candidate->GetPositionX(), candidate->GetPositionY(), candidate->GetPositionZ()))
        { ++filtered; ++rLos; continue; }

        if (Creature* c = candidate->ToCreature())
        {
            // R5: never target far-higher creatures or elites - ungrouped bots
            // suicide-loop against them (e.g. the elite-50 Stormwind Sewer
            // Beast camped by level-1 Elwynn bots at the city gate) for zero
            // loot and endless corpse runs.
            if ((int32)c->GetLevel() - (int32)bot->GetLevel() > (int32)sPlayerbotAIConfig.maxTargetLevelDiff)
            { ++filtered; ++rLevel; continue; }  // too many levels above us
            if (c->GetCreatureInfo() && c->GetCreatureInfo()->rank > CREATURE_ELITE_NORMAL)
            { ++filtered; ++rElite; continue; }  // elite/rare/worldboss - never grind ungrouped
            // AC+Shyalya pattern: skip targets on a very different height
            // (cliffs, overhead paths, underground) - prevents cliff-chasing.
            if (fabs(bot->GetPositionZ() - c->GetPositionZ()) > 30.0f)
            { ++filtered; ++rZdiff; continue; }

            // ---- tap-claim contention model (AC AttackersValue) ----
            if (c->HasLootRecipient())
            {
                if (!c->IsTappedBy(bot))
                { ++filtered; ++rTapped; continue; }  // tapped by someone else: their kill
                // tapped by me: valid, fall through
            }
            else
            {
                // Untapped. If the creature is actively fighting another
                // player's character/pet OUTSIDE MY GROUP, that is a pull in
                // progress - leave it (they tap within a swing). Anything else
                // (no victim, victim is wildlife/guard/NPC, victim is me, or
                // victim is a groupmate's pull) is fair game (AC clause 2d:
                // a groupmate's fight belongs to us - grouped bots converge
                // and assist; dungeon/raid groups depend on this).
                Unit* victim = c->GetVictim();
                if (victim && victim != bot)
                {
                    Player* victimOwner = victim->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if (victimOwner && victimOwner != bot &&
                        (!bot->GetGroup() || bot->GetGroup() != victimOwner->GetGroup()))
                    { ++filtered; ++rVictim; continue; }  // another player's pull in progress
                }
            }
        }

        float dist = GetDistance2d(bot, candidate);

        // R5c XP-primary scoring: below-level mobs look farther
        float effDist = dist;
        if (candidate->GetLevel() < bot->GetLevel())
            effDist += (float)(bot->GetLevel() - candidate->GetLevel()) * sPlayerbotAIConfig.xpLevelPenalty;

        if (effDist < bestEffDist)
        { bestEffDist = effDist; best = candidate; }
    }

    LOG_DEBUG("playerbots", "%s [SelectNearestSafeTarget] botlvl=%u candidates=%u filtered=%u (fr=%u xp=%u lvl=%u eli=%u z=%u tap=%u vic=%u los=%u) best=%s lvl=%u dist=%.1f effDist=%.1f",
        bot->GetName(), bot->GetLevel(), (uint32)collector.candidates.size(), filtered,
        rFriendly, rNoXP, rLevel, rElite, rZdiff, rTapped, rVictim, rLos,
        best ? best->GetName() : "none", best ? best->GetLevel() : 0,
        best ? GetDistance2d(bot, best) : 0.0f, bestEffDist);

    // R5d: feed the idle-relocation clock (success marks; failure registers
    // first-seen so the grace window starts)
    if (best)
        MarkViableGrindTargetSeen(bot);
    else
        (void)SecondsWithoutViableGrindTarget(bot);

    return best;
}

