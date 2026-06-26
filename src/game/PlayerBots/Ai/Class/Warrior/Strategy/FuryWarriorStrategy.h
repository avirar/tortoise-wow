/**
 * @file FuryWarriorStrategy.h
 * @brief Fury warrior combat strategy
 *
 * Fury rotation priority (Turtle WoW 1.18.1):
 * 1. Execute (below 20%)
 * 2. Bloodthirst (if talented, on cooldown)
 * 3. Whirlwind (AoE or filler)
 * 4. Heroic Strike (when rage >= 20)
 * 5. Rend (maintain debuff)
 * 6. Thunder Clap (maintain debuff)
 * 7. Auto-attack (default)
 *
 * Stance: Berserker Stance preferred
 */
#ifndef _PLAYERBOT_FURY_WARRIOR_STRATEGY_H
#define _PLAYERBOT_FURY_WARRIOR_STRATEGY_H

#include "Strategy/GenericWarriorStrategy.h"

class PlayerBotAI;

class FuryWarriorStrategy : public GenericWarriorStrategy
{
public:
    FuryWarriorStrategy(PlayerBotAI* botAI);
    virtual ~FuryWarriorStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;

protected:
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const override;
};

#endif // _PLAYERBOT_FURY_WARRIOR_STRATEGY_H
