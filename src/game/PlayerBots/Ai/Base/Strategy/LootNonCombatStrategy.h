#ifndef _PLAYERBOT_LOOTNONCOMBATSTRATEGY_H
#define _PLAYERBOT_LOOTNONCOMBATSTRATEGY_H

#include "Strategy.h"

class PlayerBotAI;

class LootNonCombatStrategy : public Strategy
{
public:
    LootNonCombatStrategy(PlayerBotAI* botAI) : Strategy(botAI) {}
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "loot"; }
};

#endif
