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

    // AC DeadStrategy pattern: auto release, find corpse, revive, accept resurrect, pop
    triggers.push_back(
        new TriggerNode("can self resurrect", { NextAction("self resurrect", 12.0f) }));
    triggers.push_back(
        new TriggerNode("often", { NextAction("auto release", 1.0f) }));
    triggers.push_back(
        new TriggerNode("dead", { NextAction("find corpse", 1.0f) }));
    triggers.push_back(new TriggerNode(
        "corpse near", { NextAction("revive from corpse", 0.0f) }));
    triggers.push_back(new TriggerNode(
        "resurrect request", { NextAction("accept resurrect", 1.0f) }));
    triggers.push_back(
        new TriggerNode("falling far", { NextAction("repop", 2.0f) }));
}
