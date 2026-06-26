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
