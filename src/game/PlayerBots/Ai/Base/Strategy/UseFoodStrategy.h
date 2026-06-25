#ifndef PLAYERBOT_USE_FOOD_STRATEGY_H
#define PLAYERBOT_USE_FOOD_STRATEGY_H

#include "Strategy.h"

class PlayerBotAI;

class UseFoodStrategy : public Strategy
{
public:
    UseFoodStrategy(PlayerBotAI* botAI) : Strategy(botAI) {}
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif
