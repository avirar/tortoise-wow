/**
 * @file GenericPaladinStrategy.cpp
 * @brief Base paladin combat strategy implementation (Turtle WoW 1.18.1)
 */
#include "GenericPaladinStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"

GenericPaladinStrategy::GenericPaladinStrategy(PlayerBotAI* botAI)
    : MeleeCombatStrategy(botAI)
{
}

std::vector<NextAction> GenericPaladinStrategy::getDefaultActions()
{
    // "cast spell" (paladin spells via SelectOffensiveSpell; falls back to
    // melee auto-attack) + reach-melee positioning.
    return MeleeCombatStrategy::getDefaultActions();
}

void GenericPaladinStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    MeleeCombatStrategy::InitTriggers(triggers);

    // Melee stun: Hammer of Justice (gated by can-hammer: has spell, no CD,
    // in melee range, target not already stunned)
    triggers.push_back(new TriggerNode(
        "can hammer of justice",
        { NextAction("hammer of justice", 16.0f) }
    ));

    // Survivability: self-heal when health is low (Holy Light)
    triggers.push_back(new TriggerNode(
        "low health",
        { NextAction("heal self", 20.0f) }
    ));

    InitSpecTriggers(triggers);
}
