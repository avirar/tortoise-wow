#include "DpsAssistStrategy.h"

#include "PlayerBotAI.h"
#include "Trigger/TriggerNode.h"

DpsAssistStrategy::DpsAssistStrategy(PlayerBotAI* botAI)
    : Strategy(botAI)
{
}

void DpsAssistStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(
        new TriggerNode("not dps target active", { NextAction("dps assist", 50.0f) }));
}
