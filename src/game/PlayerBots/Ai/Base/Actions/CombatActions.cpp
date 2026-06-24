#include "CombatActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "AiObjectContext.h"
#include "Value.h"

EnterCombatAction::EnterCombatAction(PlayerBotAI* botAI)
    : Action(botAI, "enter combat")
{
}

bool EnterCombatAction::Execute([[maybe_unused]] Event event)
{
    Value<bool>* inCombat = GetAiObjectContext()->GetValue<bool>("in combat");
    if (inCombat)
        inCombat->Set(true);
    return true;
}

LeaveCombatAction::LeaveCombatAction(PlayerBotAI* botAI)
    : Action(botAI, "leave combat")
{
}

bool LeaveCombatAction::Execute([[maybe_unused]] Event event)
{
    Value<bool>* inCombat = GetAiObjectContext()->GetValue<bool>("in combat");
    if (inCombat)
        inCombat->Set(false);

    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
    if (targetValue)
        targetValue->Set(nullptr);

    if (bot->IsInCombat())
        bot->CombatStop();

    return true;
}
