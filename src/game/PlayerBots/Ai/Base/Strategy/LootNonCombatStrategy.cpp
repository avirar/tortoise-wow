#include "LootNonCombatStrategy.h"
#include "PlayerBotAI.h"
#include "Trigger/TriggerNode.h"

void LootNonCombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("loot available", { NextAction("loot", 6.0f) }));
    triggers.push_back(new TriggerNode("far from loot target", { NextAction("move to loot", 7.0f) }));
    triggers.push_back(new TriggerNode("can loot", { NextAction("open loot", 8.0f) }));
    triggers.push_back(new TriggerNode("often", { NextAction("add all loot", 5.0f) }));
}
