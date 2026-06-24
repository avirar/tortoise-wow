#ifndef _PLAYERBOT_GRINDING_STRATEGY_H
#define _PLAYERBOT_GRINDING_STRATEGY_H

#include "Strategy.h"

class PlayerBotAI;

class GrindingStrategy : public Strategy
{
public:
    GrindingStrategy(PlayerBotAI* botAI);
    virtual ~GrindingStrategy() {}

    virtual std::vector<NextAction> getDefaultActions() override;
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::string const getName() { return "grind"; }
};

#endif
