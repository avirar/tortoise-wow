#include "GrindingStrategy.h"

#include "PlayerBotAI.h"
#include "Trigger/TriggerNode.h"

GrindingStrategy::GrindingStrategy(PlayerBotAI* botAI)
    : Strategy(botAI)
{
}

std::vector<NextAction> GrindingStrategy::getDefaultActions()
{
    return std::vector<NextAction>();
}

void GrindingStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("no target", { NextAction("attack anything", 4.0f) }));
}
