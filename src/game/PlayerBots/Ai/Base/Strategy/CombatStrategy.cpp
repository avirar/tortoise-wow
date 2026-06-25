#include "CombatStrategy.h"

#include "Trigger/TriggerNode.h"

CombatStrategy::CombatStrategy(PlayerBotAI* botAI) : Strategy(botAI)
{
}

std::vector<NextAction> CombatStrategy::getDefaultActions()
{
    return std::vector<NextAction>{
        NextAction("dps assist", ACTION_DEFAULT)
    };
}

void CombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "invalid target",
        { NextAction("drop target", 99) }
    ));
    triggers.push_back(new TriggerNode(
        "no target",
        { NextAction("attack anything", ACTION_IDLE) }
    ));
    triggers.push_back(new TriggerNode(
        "not facing target",
        { NextAction("set facing", ACTION_MOVE + 7) }
    ));
}
