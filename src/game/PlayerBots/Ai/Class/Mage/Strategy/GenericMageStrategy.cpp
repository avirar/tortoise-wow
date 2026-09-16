/**
 * @file GenericMageStrategy.cpp
 * @brief Base mage combat strategy implementation (Turtle WoW 1.18.1)
 *
 * Default actions inherit from RangedCombatStrategy (cast spell / shoot /
 * flee-when-close). The class offensive table (PlayerBotAI::SelectOffensiveSpell)
 * already resolves the highest known rank of Frostbolt/Fireball/Fire Blast/
 * Frost Nova/Arcane Explosion for CLASS_MAGE, so "cast spell" is the nuke.
 * We add the Polymorph CC trigger on top.
 */
#include "GenericMageStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"

#include "../MageTriggers.h"

GenericMageStrategy::GenericMageStrategy(PlayerBotAI* botAI)
    : RangedCombatStrategy(botAI)
{
}

std::vector<NextAction> GenericMageStrategy::getDefaultActions()
{
    // RangedCombatStrategy provides "cast spell" (mage nukes) + flee-when-close.
    // Spec strategies can push higher-priority spec actions via GetSpecDefaultActions().
    std::vector<NextAction> actions = RangedCombatStrategy::getDefaultActions();
    for (auto const& pair : GetSpecDefaultActions())
        actions.push_back(NextAction(pair.first, pair.second));
    return actions;
}

void GenericMageStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    RangedCombatStrategy::InitTriggers(triggers);

    // CC: sheep the first target we can (gates on level + not-already-sheeped)
    triggers.push_back(new TriggerNode(
        "can polymorph",
        { NextAction("polymorph", 15.0f) }
    ));

    // Spec-specific triggers
    InitSpecTriggers(triggers);
}
