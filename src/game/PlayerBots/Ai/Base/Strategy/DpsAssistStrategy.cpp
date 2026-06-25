#include "DpsAssistStrategy.h"

#include "PlayerBotAI.h"
#include "Trigger/TriggerNode.h"

DpsAssistStrategy::DpsAssistStrategy(PlayerBotAI* botAI)
    : Strategy(botAI)
{
}

std::vector<NextAction> DpsAssistStrategy::getDefaultActions()
{
    // Always push "dps assist" so bot keeps chasing/attacking current target
    // Trigger "not dps target active" handles target SWITCHING (priority 50)
    // Default action handles continuous chase/attack (priority ACTION_DEFAULT)
    return std::vector<NextAction>{
        NextAction("dps assist", ACTION_DEFAULT)
    };
}

void DpsAssistStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(
        new TriggerNode("not dps target active", { NextAction("dps assist", 50.0f) }));
}
