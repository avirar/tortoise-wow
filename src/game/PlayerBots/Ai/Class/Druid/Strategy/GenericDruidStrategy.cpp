/**
 * @file GenericDruidStrategy.cpp
 * @brief Base druid combat strategy implementation (Turtle WoW 1.18.1)
 */
#include "GenericDruidStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"

GenericDruidStrategy::GenericDruidStrategy(PlayerBotAI* botAI)
    : MeleeCombatStrategy(botAI)
{
}

std::vector<NextAction> GenericDruidStrategy::getDefaultActions()
{
    // "cast spell" (druid nukes via SelectOffensiveSpell; melee fallback)
    // + reach-melee positioning.
    return MeleeCombatStrategy::getDefaultActions();
}

void GenericDruidStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    MeleeCombatStrategy::InitTriggers(triggers);

    // Survivability: self-heal when health is low (Regrowth)
    triggers.push_back(new TriggerNode(
        "low health",
        { NextAction("heal self", 20.0f) }
    ));

    InitSpecTriggers(triggers);
}
