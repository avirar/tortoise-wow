/**
 * @file TankWarriorStrategy.cpp
 * @brief Protection (tank) warrior combat strategy implementation
 */
#include "TankWarriorStrategy.h"

#include "Trigger/TriggerNode.h"
#include "Value/Value.h"
#include "Spells/SpellAuras.h"

#include "../WarriorTriggers.h"

TankWarriorStrategy::TankWarriorStrategy(PlayerBotAI* botAI)
    : GenericWarriorStrategy(botAI)
{
}

void TankWarriorStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    GenericWarriorStrategy::InitTriggers(triggers);
}

void TankWarriorStrategy::InitSpecTriggers(std::vector<TriggerNode*>& triggers)
{
    // Switch to Defensive Stance
    triggers.push_back(new TriggerNode(
        "not in defensive stance",
        { NextAction("defensive stance", 9.5f) }
    ));

    // Shield Slam (Prot talent)
    triggers.push_back(new TriggerNode(
        "shield slam ready",
        { NextAction("shield slam", 7.0f) }
    ));

    // Revenge (on block)
    triggers.push_back(new TriggerNode(
        "revenge ready",
        { NextAction("revenge", 6.5f) }
    ));

    // Sunder Armor maintenance (tank priority: keep armor reduced)
    triggers.push_back(new TriggerNode(
        "sunder armor needed",
        { NextAction("sunder armor", 5.0f) }
    ));

    // Taunt (maintain aggro)
    triggers.push_back(new TriggerNode(
        "taunt needed",
        { NextAction("taunt", 8.0f) }
    ));

    // Shield Block (defensive cooldown)
    triggers.push_back(new TriggerNode(
        "shield block ready",
        { NextAction("shield block", 4.0f) }
    ));

    // Intimidating Shout (emergency, multiple targets)
    triggers.push_back(new TriggerNode(
        "intimidating shout ready",
        { NextAction("intimidating shout", 3.5f) }
    ));

    // Demoralizing Shout (debuff for tanking)
    triggers.push_back(new TriggerNode(
        "demoralizing shout needed",
        { NextAction("demoralizing shout", 4.5f) }
    ));
}

std::vector<std::pair<std::string, float>> TankWarriorStrategy::GetSpecDefaultActions() const
{
    return {
        {"execute", 10.0f},
        {"taunt", 8.0f},
        {"shield slam", 7.0f},
        {"revenge", 6.5f},
        {"thunder clap", 5.5f},
        {"sunder armor", 5.0f},
        {"demoralizing shout", 4.5f},
        {"shield block", 4.0f},
        {"battle shout", 8.0f},
        {"defensive stance", 9.5f},
        {"intimidating shout", 3.5f},
    };
}
