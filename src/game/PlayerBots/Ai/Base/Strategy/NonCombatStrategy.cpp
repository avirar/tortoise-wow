#include "NonCombatStrategy.h"

NonCombatStrategy::NonCombatStrategy(PlayerBotAI* botAI) : Strategy(botAI)
{
}

std::vector<NextAction> NonCombatStrategy::getDefaultActions()
{
    // AC FollowMasterStrategy pattern: "follow" at priority 1.0f
    return std::vector<NextAction>{
        NextAction("follow", 1.0f)
    };
}

void NonCombatStrategy::InitTriggers(std::vector<TriggerNode*>& /*triggers*/)
{
}
