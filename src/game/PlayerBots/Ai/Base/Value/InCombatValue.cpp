#include "InCombatValue.h"

#include "PlayerBotAI.h"
#include "Player.h"

InCombatValue::InCombatValue(PlayerBotAI* botAI)
    : BoolCalculatedValue(botAI, "in combat")
{
}

bool InCombatValue::Calculate()
{
    if (!botAI || !botAI->me)
        return false;

    return botAI->me->IsInCombat();
}
