#include "LootNonCombatStrategy.h"
#include "PlayerBotAI.h"
#include "Trigger/TriggerNode.h"

void LootNonCombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("loot available", { NextAction("loot", 6.0f) }));
    triggers.push_back(new TriggerNode("far from loot target", { NextAction("move to loot", 7.0f) }));
    triggers.push_back(new TriggerNode("can loot", { NextAction("open loot", 8.0f) }));
    triggers.push_back(new TriggerNode("loot open", { NextAction("store loot", 8.5f) }));
    // AC pattern: "often" periodic equip check (approximates AC's packet trigger)
    triggers.push_back(new TriggerNode("often", { NextAction("add all loot", 5.0f) }));
    triggers.push_back(new TriggerNode("often", { NextAction("equip upgrades", 0.5f) }));
    // AC WorldPacketHandlerStrategy pattern: item push result fires when items added to inventory
    triggers.push_back(new TriggerNode("item push result", { NextAction("equip upgrades", 9.0f) }));
}
