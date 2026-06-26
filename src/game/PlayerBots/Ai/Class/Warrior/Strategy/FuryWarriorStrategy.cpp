/**
 * @file FuryWarriorStrategy.cpp
 * @brief Fury warrior combat strategy implementation
 */
#include "FuryWarriorStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"

#include "../WarriorTriggers.h"

FuryWarriorStrategy::FuryWarriorStrategy(PlayerBotAI* botAI)
    : GenericWarriorStrategy(botAI)
{
}

void FuryWarriorStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    GenericWarriorStrategy::InitTriggers(triggers);
}

void FuryWarriorStrategy::InitSpecTriggers(std::vector<TriggerNode*>& triggers)
{
    // Switch to Berserker Stance
    triggers.push_back(new TriggerNode(
        "not in berserker stance",
        { NextAction("berserker stance", 9.5f) }
    ));

    // Bloodthirst (Fury talent - on cooldown)
    triggers.push_back(new TriggerNode(
        "bloodthirst ready",
        { NextAction("bloodthirst", 7.0f) }
    ));

    // Whirlwind (AoE or high-rage filler)
    triggers.push_back(new TriggerNode(
        "whirlwind ready",
        { NextAction("whirlwind", 5.0f) }
    ));

    // Heroic Strike (filler when rage is high)
    triggers.push_back(new TriggerNode(
        "heroic strike ready",
        { NextAction("heroic strike", 5.5f) }
    ));

    // Death Wish (major cooldown)
    triggers.push_back(new TriggerNode(
        "death wish ready",
        { NextAction("death wish", 3.5f) }
    ));

    // Bloodrage when rage is low (generate rage)
    triggers.push_back(new TriggerNode(
        "bloodrage needed",
        { NextAction("bloodrage", 3.0f) }
    ));
}

std::vector<std::pair<std::string, float>> FuryWarriorStrategy::GetSpecDefaultActions() const
{
    return {
        {"execute", 10.0f},
        {"bloodthirst", 7.0f},
        {"heroic strike", 5.5f},
        {"whirlwind", 5.0f},
        {"rend", 5.0f},
        {"thunder clap", 4.5f},
        {"battle shout", 8.0f},
        {"berserker stance", 9.5f},
        {"death wish", 3.5f},
        {"bloodrage", 3.0f},
    };
}
