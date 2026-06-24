#ifndef _PLAYERBOT_COMBAT_STRATEGY_H
#define _PLAYERBOT_COMBAT_STRATEGY_H

#include "Strategy.h"

class PlayerBotAI;

class CombatStrategy : public Strategy
{
public:
    CombatStrategy(PlayerBotAI* botAI);
    virtual ~CombatStrategy() {}

    virtual std::vector<NextAction> getDefaultActions() override;
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::string const getName() { return "combat"; }
    virtual uint32 GetType() const { return STRATEGY_TYPE_COMBAT; }
};

#endif
