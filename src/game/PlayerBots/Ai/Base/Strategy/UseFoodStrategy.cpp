#include "UseFoodStrategy.h"

#include "TriggerNode.h"
#include "PlayerBotAI.h"

void UseFoodStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("low health", { NextAction("food", 3.0f) }));
    triggers.push_back(new TriggerNode("low mana", { NextAction("drink", 3.0f) }));
}
