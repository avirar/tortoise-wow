#ifndef _PLAYERBOT_MELEE_COMBAT_STRATEGY_H
#define _PLAYERBOT_MELEE_COMBAT_STRATEGY_H

#include "CombatStrategy.h"

class PlayerBotAI;

class MeleeCombatStrategy : public CombatStrategy
{
public:
    MeleeCombatStrategy(PlayerBotAI* botAI);
    virtual ~MeleeCombatStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::string const getName() { return "close"; }
    virtual uint32 GetType() const { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_MELEE; }
};

class SetBehindCombatStrategy : public CombatStrategy
{
public:
    SetBehindCombatStrategy(PlayerBotAI* botAI);
    virtual ~SetBehindCombatStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::string const getName() { return "behind"; }
    virtual uint32 GetType() const { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_MELEE; }
};

#endif
