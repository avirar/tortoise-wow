/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#ifndef _PLAYERBOT_DEADSTRATEGY_H
#define _PLAYERBOT_DEADSTRATEGY_H

#include "Ai/Base/Strategy/NonCombatStrategy.h"

class PlayerBotAI;

class DeadStrategy : public NonCombatStrategy
{
public:
    DeadStrategy(PlayerBotAI* botAI) : NonCombatStrategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "dead"; }
};

#endif
