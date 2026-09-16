/**
 * @file GenericShamanStrategy.cpp
 * @brief Base shaman combat strategy implementation (Turtle WoW 1.18.1)
 */
#include "GenericShamanStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"

GenericShamanStrategy::GenericShamanStrategy(PlayerBotAI* botAI)
    : RangedCombatStrategy(botAI)
{
}

std::vector<NextAction> GenericShamanStrategy::getDefaultActions()
{
    // "cast spell" (shaman nukes via SelectOffensiveSpell)
    return RangedCombatStrategy::getDefaultActions();
}

void GenericShamanStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    RangedCombatStrategy::InitTriggers(triggers);

    // Survivability: self-heal when health is low (Healing Wave)
    triggers.push_back(new TriggerNode(
        "low health",
        { NextAction("heal self", 20.0f) }
    ));

    InitSpecTriggers(triggers);
}
