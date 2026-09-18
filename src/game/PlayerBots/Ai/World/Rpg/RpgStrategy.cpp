/*
 * Rpg strategy — R7 L2. See header.
 */

#include "RpgStrategy.h"
#include "Trigger/TriggerNode.h"

std::vector<NextAction> RpgStrategy::getDefaultActions()
{
    // AC: "the relevance should be greater than grind" — the state-machine driver
    // runs every tick, before the status actions.
    return std::vector<NextAction>(1, NextAction("rpg status update", 11.0f));
}

void RpgStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(
        new TriggerNode(
            "go grind status",
            { NextAction("rpg go grind", 3.0f) }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "wander random status",
            { NextAction("rpg wander random", 3.0f) }
        )
    );
    // REST / IDLE need no movement action (the bot sits while REST, idles while
    // IDLE — handled by RandomChangeStatus + the status-update action).
    // DO_QUEST: deferred to L3+ (quest POI pipeline).
}

void RpgStrategy::InitMultipliers(std::vector<Multiplier*>&)
{
}
