#include "RangedCombatStrategy.h"

#include "Trigger/TriggerNode.h"

RangedCombatStrategy::RangedCombatStrategy(PlayerBotAI* botAI) : CombatStrategy(botAI)
{
}

void RangedCombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    CombatStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode("enemy too close for spell",
                                        { NextAction("flee", ACTION_MOVE + 4) }));
}
