#include "CombatStrategy.h"

#include "Trigger/TriggerNode.h"

CombatStrategy::CombatStrategy(PlayerBotAI* botAI) : Strategy(botAI)
{
}

std::vector<NextAction> CombatStrategy::getDefaultActions()
{
    // AC pattern: CombatStrategy provides "cast spell" as default action.
    // CastSpellAction calls SelectOffensiveSpell() which covers all 9 classes.
    // Falls back to melee auto-attack if no spell is available.
    // Relevance ACTION_DEFAULT so positioning triggers fire first.
    return std::vector<NextAction>{ NextAction("cast spell", ACTION_DEFAULT) };
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
