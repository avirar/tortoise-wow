#include "WanderStrategy.h"

#include "PlayerBotAI.h"
#include "Trigger/TriggerNode.h"

WanderStrategy::WanderStrategy(PlayerBotAI* botAI)
    : Strategy(botAI)
{
}

std::vector<NextAction> WanderStrategy::getDefaultActions()
{
    return std::vector<NextAction>();
}

void WanderStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("often", { NextAction("move random", ACTION_IDLE) }));
}
