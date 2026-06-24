#include "NonCombatStrategy.h"

NonCombatStrategy::NonCombatStrategy(PlayerBotAI* botAI) : Strategy(botAI)
{
}

std::vector<NextAction> NonCombatStrategy::getDefaultActions()
{
    return std::vector<NextAction>{
        NextAction("follow", ACTION_IDLE)
    };
}

void NonCombatStrategy::InitTriggers(std::vector<TriggerNode*>& /*triggers*/)
{
}
