#include "NonCombatActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "ServerFacade.h"
#include "AiObjectContext.h"
#include "Value.h"

SetFacingAction::SetFacingAction(PlayerBotAI* botAI)
    : Action(botAI, "set facing")
{
}

bool SetFacingAction::Execute([[maybe_unused]] Event event)
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (!targetValue)
        return false;

    Unit* target = targetValue->Get();
    if (!target || !target->IsAlive())
        return false;

    bot->SetFacingToObject(target);
    return true;
}

SetBehindAction::SetBehindAction(PlayerBotAI* botAI)
    : MovementAction(botAI, "set behind")
{
}

bool SetBehindAction::Execute([[maybe_unused]] Event event)
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (!targetValue)
        return false;

    Unit* target = targetValue->Get();
    if (!target || !target->IsAlive())
        return false;

    float x, y, z;
    float targetO = target->GetOrientation();
    float dist = 3.0f;

    x = target->GetPositionX() - cos(targetO) * dist;
    y = target->GetPositionY() - sin(targetO) * dist;
    z = target->GetPositionZ();

    return MoveTo(x, y, z);
}

bool SetBehindAction::isUseful()
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (!targetValue)
        return false;

    Unit* target = targetValue->Get();
    if (!target || !target->IsAlive())
        return false;

    float botO = bot->GetOrientation();
    float targetO = target->GetOrientation();

    float angle = abs(botO - targetO);
    while (angle > M_PI)
        angle -= M_PI * 2;
    while (angle < -M_PI)
        angle += M_PI * 2;

    return abs(angle) > M_PI / 2.0f;
}
