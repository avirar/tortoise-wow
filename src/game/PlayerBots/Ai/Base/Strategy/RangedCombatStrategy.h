#ifndef _PLAYERBOT_RANGED_COMBAT_STRATEGY_H
#define _PLAYERBOT_RANGED_COMBAT_STRATEGY_H

#include "CombatStrategy.h"

class PlayerBotAI;

class RangedCombatStrategy : public CombatStrategy
{
public:
    RangedCombatStrategy(PlayerBotAI* botAI);
    virtual ~RangedCombatStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::string const getName() { return "ranged"; }
    virtual uint32 GetType() const { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_RANGED; }
};

#endif
