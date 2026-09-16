/**
 * @file GenericWarriorStrategy.cpp
 * @brief Base warrior combat strategy implementation
 */
#include "GenericWarriorStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"

#include "../WarriorTriggers.h"
#include "../WarriorAiObjectContext.h"

GenericWarriorStrategy::GenericWarriorStrategy(PlayerBotAI* botAI)
    : CombatStrategy(botAI)
{
}

std::vector<NextAction> GenericWarriorStrategy::getDefaultActions()
{
    // AC pattern: spec strategies define default action priorities via GetSpecDefaultActions()
    // CombatStrategy::getDefaultActions() provides "cast spell" — we add spec actions on top
    std::vector<NextAction> actions = CombatStrategy::getDefaultActions();
    for (auto const& pair : GetSpecDefaultActions())
    {
        actions.push_back(NextAction(pair.first, pair.second));
    }
    return actions;
}

void GenericWarriorStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    CombatStrategy::InitTriggers(triggers);

    // Stance management
    triggers.push_back(new TriggerNode(
        "not in battle stance",
        { NextAction("battle stance", 9.0f) }
    ));

    // Buff management
    triggers.push_back(new TriggerNode(
        "battle shout expired",
        { NextAction("battle shout", 8.0f) }
    ));

    // Debuff management (common to all specs)
    triggers.push_back(new TriggerNode(
        "rend expired",
        { NextAction("rend", 5.0f) }
    ));

    triggers.push_back(new TriggerNode(
        "thunder clap expired",
        { NextAction("thunder clap", 4.5f) }
    ));

    // Execute (finisher - highest priority when target is low)
    triggers.push_back(new TriggerNode(
        "execute ready",
        { NextAction("execute", 10.0f) }
    ));

    // Spec-specific triggers
    InitSpecTriggers(triggers);
}
