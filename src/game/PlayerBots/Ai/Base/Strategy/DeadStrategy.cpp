/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#include "DeadStrategy.h"
#include "TriggerContext.h"
#include "Engine/Trigger/TriggerNode.h"

void DeadStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    NonCombatStrategy::InitTriggers(triggers);

    // AC DeadStrategy pattern: relevance=100, revive=relevance-1, pop=relevance+1, self-res=relevance+2
    triggers.push_back(
        new TriggerNode("can self resurrect", { NextAction("self resurrect", 102.0f) }));
    triggers.push_back(
        new TriggerNode("often", { NextAction("auto release", 100.0f) }));
    triggers.push_back(
        new TriggerNode("dead", { NextAction("find corpse", 100.0f) }));
    triggers.push_back(new TriggerNode(
        "corpse near", { NextAction("revive from corpse", 99.0f) }));
    triggers.push_back(new TriggerNode(
        "resurrect request", { NextAction("accept resurrect", 100.0f) }));
    triggers.push_back(
        new TriggerNode("falling far", { NextAction("repop", 101.0f) }));
}
