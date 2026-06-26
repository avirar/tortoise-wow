#include "CombatStrategy.h"

#include "Trigger/TriggerNode.h"

CombatStrategy::CombatStrategy(PlayerBotAI* botAI) : Strategy(botAI)
{
}

std::vector<NextAction> CombatStrategy::getDefaultActions()
{
    // AC pattern: CombatStrategy has no default actions.
    // Solo bots retarget via "no target" → "attack anything" (reads "grind target").
    // Group bots use DpsAssistStrategy "not dps target active" trigger.
    return std::vector<NextAction>{};
}

void CombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // AC pattern: CombatStrategy has only positioning/housekeeping triggers.
    // No "no target" trigger — retargeting happens on NON_COMBAT engine via GrindingStrategy.
    triggers.push_back(new TriggerNode(
        "invalid target",
        { NextAction("drop target", 99) }
    ));
    triggers.push_back(new TriggerNode(
        "not facing target",
        { NextAction("set facing", ACTION_MOVE + 7) }
    ));
}
