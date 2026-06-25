#include "GrindingStrategy.h"

#include "PlayerBotAI.h"
#include "Trigger/TriggerNode.h"
#include "Logging.h"
#include "Log.h"

GrindingStrategy::GrindingStrategy(PlayerBotAI* botAI)
    : Strategy(botAI)
{
}

std::vector<NextAction> GrindingStrategy::getDefaultActions()
{
    return std::vector<NextAction>{ NextAction("drink", 4.2f), NextAction("food", 4.1f) };
}

void GrindingStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    LOG_DEBUG("playerbots", "[GrindingStrategy::InitTriggers] before: %u triggers", triggers.size());
    // Clear stale dead targets (matches CombatStrategy pattern)
    triggers.push_back(new TriggerNode("invalid target", { NextAction("drop target", 99.0f) }));
    // Find new target when none held (AC GrindingStrategy pattern)
    triggers.push_back(new TriggerNode("no target", { NextAction("attack anything", 4.0f) }));
    triggers.push_back(new TriggerNode("loot available", { NextAction("loot", 6.0f) }));
    triggers.push_back(new TriggerNode("far from loot target", { NextAction("move to loot", 7.0f) }));
    triggers.push_back(new TriggerNode("can loot", { NextAction("open loot", 8.0f) }));
    triggers.push_back(new TriggerNode("often", { NextAction("add all loot", 5.0f) }));
    // Food/drink (AC UseFoodStrategy pattern)
    triggers.push_back(new TriggerNode("low health", { NextAction("food", 3.0f) }));
    triggers.push_back(new TriggerNode("low mana", { NextAction("drink", 3.0f) }));
    LOG_DEBUG("playerbots", "[GrindingStrategy::InitTriggers] after: %u triggers", triggers.size());
}
