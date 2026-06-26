/**
 * @file TankWarriorStrategy.h
 * @brief Protection (tank) warrior combat strategy
 *
 * Prot rotation priority (Turtle WoW 1.18.1):
 * 1. Execute (below 20%)
 * 2. Revenge (on block proc)
 * 3. Shield Slam (if talented)
 * 4. Thunder Clap (maintain debuff)
 * 5. Sunder Armor (maintain debuff, 5 stacks)
 * 6. Taunt (maintain aggro)
 * 7. Shield Block (defensive cooldown)
 * 8. Heroic Strike (filler when rage is high)
 * 9. Auto-attack (default)
 *
 * Stance: Defensive Stance preferred
 */
#ifndef _PLAYERBOT_TANK_WARRIOR_STRATEGY_H
#define _PLAYERBOT_TANK_WARRIOR_STRATEGY_H

#include "Strategy/GenericWarriorStrategy.h"

class PlayerBotAI;

class TankWarriorStrategy : public GenericWarriorStrategy
{
public:
    TankWarriorStrategy(PlayerBotAI* botAI);
    virtual ~TankWarriorStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;

protected:
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const override;
};

#endif // _PLAYERBOT_TANK_WARRIOR_STRATEGY_H
