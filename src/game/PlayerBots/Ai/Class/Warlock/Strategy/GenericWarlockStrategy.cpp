/**
 * @file GenericWarlockStrategy.cpp
 * @brief Base warlock combat strategy implementation (Turtle WoW 1.18.1)
 */
#include "GenericWarlockStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"

GenericWarlockStrategy::GenericWarlockStrategy(PlayerBotAI* botAI)
    : RangedCombatStrategy(botAI)
{
}

std::vector<NextAction> GenericWarlockStrategy::getDefaultActions()
{
    // "cast spell" (warlock nukes via SelectOffensiveSpell) + flee-when-close
    return RangedCombatStrategy::getDefaultActions();
}

void GenericWarlockStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    RangedCombatStrategy::InitTriggers(triggers);

    // CC: fear the target (gated by can-fear: has spell, no CD, not already
    // feared). Fear makes the target flee — a soft CC for survivability.
    triggers.push_back(new TriggerNode(
        "can fear",
        { NextAction("fear", 15.0f) }
    ));

    InitSpecTriggers(triggers);
}
