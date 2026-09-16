/**
 * @file GenericPriestStrategy.cpp
 * @brief Base priest combat strategy implementation (Turtle WoW 1.18.1)
 */
#include "GenericPriestStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"

GenericPriestStrategy::GenericPriestStrategy(PlayerBotAI* botAI)
    : RangedCombatStrategy(botAI)
{
}

std::vector<NextAction> GenericPriestStrategy::getDefaultActions()
{
    // "cast spell" (priest nukes via SelectOffensiveSpell) + flee-when-close
    return RangedCombatStrategy::getDefaultActions();
}

void GenericPriestStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    RangedCombatStrategy::InitTriggers(triggers);

    // Survivability: self-heal when health is low (Greater Heal)
    triggers.push_back(new TriggerNode(
        "low health",
        { NextAction("heal self", 20.0f) }
    ));

    InitSpecTriggers(triggers);
}
