#include "MeleeCombatStrategy.h"

#include "Trigger/TriggerNode.h"

MeleeCombatStrategy::MeleeCombatStrategy(PlayerBotAI* botAI) : CombatStrategy(botAI)
{
}

void MeleeCombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    CombatStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "enemy out of melee", { NextAction("reach melee", ACTION_HIGH + 1) }));
}

SetBehindCombatStrategy::SetBehindCombatStrategy(PlayerBotAI* botAI) : CombatStrategy(botAI)
{
}

void SetBehindCombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    CombatStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode("not behind target",
                                        { NextAction("set behind", ACTION_MOVE + 7) }));
}
