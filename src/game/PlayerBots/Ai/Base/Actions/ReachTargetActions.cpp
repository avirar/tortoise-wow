#include "ReachTargetActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"
#include "AiObjectContext.h"
#include "Value.h"

ReachCloseCombatAction::ReachCloseCombatAction(PlayerBotAI* botAI)
    : MovementAction(botAI, "reach melee")
{
}

bool ReachCloseCombatAction::Execute([[maybe_unused]] Event event)
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (!targetValue)
        return false;

    Unit* target = targetValue->Get();
    if (!target || !target->IsAlive())
        return false;

    bot->GetMotionMaster()->MoveChase(target);
    return true;
}

bool ReachCloseCombatAction::isUseful()
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (!targetValue)
        return false;

    Unit* target = targetValue->Get();
    if (!target || !target->IsAlive())
        return false;

    float dist = sServerFacade.GetDistance2d(bot, target);
    return sServerFacade.IsDistanceGreaterThan(dist, sPlayerbotAIConfig.meleeDistance);
}

ReachSpellCombatAction::ReachSpellCombatAction(PlayerBotAI* botAI)
    : MovementAction(botAI, "reach spell")
{
}

bool ReachSpellCombatAction::Execute([[maybe_unused]] Event event)
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (!targetValue)
        return false;

    Unit* target = targetValue->Get();
    if (!target || !target->IsAlive())
        return false;

    float dist = sServerFacade.GetDistance2d(bot, target);

    if (sServerFacade.IsDistanceLessThan(dist, sPlayerbotAIConfig.meleeDistance))
    {
        float x, y, z;
        target->GetClosePoint(x, y, z, 1.0f, sPlayerbotAIConfig.spellDistance, 0, bot);
        return MoveTo(x, y, z);
    }
    else if (sServerFacade.IsDistanceGreaterThan(dist, sPlayerbotAIConfig.spellDistance))
    {
        float x, y, z;
        target->GetClosePoint(x, y, z, 1.0f, sPlayerbotAIConfig.meleeDistance + 5.0f, 0, bot);
        return MoveTo(x, y, z);
    }

    return false;
}

bool ReachSpellCombatAction::isUseful()
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (!targetValue)
        return false;

    Unit* target = targetValue->Get();
    if (!target || !target->IsAlive())
        return false;

    float dist = sServerFacade.GetDistance2d(bot, target);

    if (sServerFacade.IsDistanceLessThan(dist, sPlayerbotAIConfig.meleeDistance))
        return true;

    if (sServerFacade.IsDistanceGreaterThan(dist, sPlayerbotAIConfig.spellDistance))
        return true;

    return false;
}

FleeAction::FleeAction(PlayerBotAI* botAI)
    : MovementAction(botAI, "flee")
{
}

bool FleeAction::Execute([[maybe_unused]] Event event)
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (!targetValue)
        return false;

    Unit* target = targetValue->Get();
    if (!target || !target->IsAlive())
        return false;

    float x = bot->GetPositionX();
    float y = bot->GetPositionY();
    float z = bot->GetPositionZ();

    float angle = bot->GetAngle(target);
    float fleeDist = sPlayerbotAIConfig.spellDistance + 10.0f;

    float newX = x + cos(angle) * fleeDist;
    float newY = y + sin(angle) * fleeDist;

    return MoveTo(newX, newY, z);
}

bool FleeAction::isUseful()
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (!targetValue)
        return false;

    Unit* target = targetValue->Get();
    if (!target || !target->IsAlive())
        return false;

    float dist = sServerFacade.GetDistance2d(bot, target);
    return sServerFacade.IsDistanceLessThan(dist, sPlayerbotAIConfig.meleeDistance);
}
