#ifndef _PLAYERBOT_NONCOMBAT_STRATEGY_H
#define _PLAYERBOT_NONCOMBAT_STRATEGY_H

#include "Strategy.h"

class PlayerBotAI;

class NonCombatStrategy : public Strategy
{
public:
    NonCombatStrategy(PlayerBotAI* botAI);
    virtual ~NonCombatStrategy() {}

    virtual std::vector<NextAction> getDefaultActions() override;
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::string const getName() { return "noncombat"; }
};

#endif
