#include "MovementActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Object.h"
#include "Group.h"
#include "Spell.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"
#include "Logging.h"
#include <cmath>

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

bool MovementAction::IsDuplicateMove(float x, float y, float z)
{
    float dist = sServerFacade.GetDistance2d(bot, x, y);
    return sServerFacade.IsDistanceLessThan(dist, 1.0f);
}

bool MovementAction::IsMovingAllowed()
{
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

bool MoveRandomAction::Execute(Event /*event*/)
{
    float distance = sPlayerbotAIConfig.sightDistance;

    for (int i = 0; i < 5; ++i)
    {
        float angle = (float)urand(0, 360) * M_PI / 180.0f;
        float dist = (float)urand(10, (uint32)distance);
        float x = bot->GetPositionX() + dist * cos(angle);
        float y = bot->GetPositionY() + dist * sin(angle);
        float z = bot->GetPositionZ();

        if (MoveTo(x, y, z))
        {
            LOG_DEBUG("playerbots", "%s [MoveRandomAction] pos=(%.1f,%.1f) -> dest=(%.1f,%.1f) dist=%.1f", 
                bot->GetName(), bot->GetPositionX(), bot->GetPositionY(), x, y, dist);
            return true;
        }
    }

    LOG_DEBUG("playerbots", "%s [MoveRandomAction] failed to find valid destination", bot->GetName());
    return false;
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
