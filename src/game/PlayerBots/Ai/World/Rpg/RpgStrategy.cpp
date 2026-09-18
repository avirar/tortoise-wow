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
    // AC NewRpgStrategy trigger list (the DO QUEST entry is L4/L5 - the do-quest
    // action walks to objectives/takers; the kills + accept/turn-in are handled
    // by the base grind strategy + SearchQuestGiverAndAcceptOrReward).
    triggers.push_back(
        new TriggerNode(
            "do quest status",
            { NextAction("rpg do quest", 3.0f) }
        )
    );
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
    // IDLE - handled by RandomChangeStatus + the status-update action).
}


void RpgStrategy::InitMultipliers(std::vector<Multiplier*>&)
{
}
