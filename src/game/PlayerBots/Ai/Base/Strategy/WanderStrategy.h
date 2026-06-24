#ifndef _PLAYERBOT_WANDER_STRATEGY_H
#define _PLAYERBOT_WANDER_STRATEGY_H

#include "Strategy.h"

class PlayerBotAI;

class WanderStrategy : public Strategy
{
public:
    WanderStrategy(PlayerBotAI* botAI);
    virtual ~WanderStrategy() {}

    virtual std::vector<NextAction> getDefaultActions() override;
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::string const getName() { return "wander"; }
};

#endif
