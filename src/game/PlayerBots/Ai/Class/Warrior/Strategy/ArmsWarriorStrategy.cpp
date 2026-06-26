/**
 * @file ArmsWarriorStrategy.cpp
 * @brief Arms warrior combat strategy implementation
 */
#include "ArmsWarriorStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"

#include "../WarriorTriggers.h"

ArmsWarriorStrategy::ArmsWarriorStrategy(PlayerBotAI* botAI)
    : GenericWarriorStrategy(botAI)
{
}

void ArmsWarriorStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    GenericWarriorStrategy::InitTriggers(triggers);
}

void ArmsWarriorStrategy::InitSpecTriggers(std::vector<TriggerNode*>& triggers)
{
    // Arms-specific triggers

    // Mortal Strike (Arms talent - Wrecking Blow)
    triggers.push_back(new TriggerNode(
        "mortal strike ready",
        { NextAction("mortal strike", 6.0f) }
    ));

    // Sunder Armor maintenance (Arms value: keep debuffs)
    triggers.push_back(new TriggerNode(
        "sunder armor needed",
        { NextAction("sunder armor", 4.0f) }
    ));

    // Heroic Strike (filler when rage is high)
    triggers.push_back(new TriggerNode(
        "heroic strike ready",
        { NextAction("heroic strike", 5.5f) }
    ));

    // Bloodrage when rage is low (generate rage)
    triggers.push_back(new TriggerNode(
        "bloodrage needed",
        { NextAction("bloodrage", 3.0f) }
    ));
}

std::vector<std::pair<std::string, float>> ArmsWarriorStrategy::GetSpecDefaultActions() const
{
    // Default action priorities for Arms spec
    // Higher priority = more important
    return {
        {"execute", 10.0f},
        {"mortal strike", 6.0f},
        {"heroic strike", 5.5f},
        {"rend", 5.0f},
        {"thunder clap", 4.5f},
        {"sunder armor", 4.0f},
        {"battle shout", 8.0f},
        {"battle stance", 9.0f},
        {"bloodrage", 3.0f},
    };
}
