/**
 * @file ArmsWarriorStrategy.h
 * @brief Arms warrior combat strategy
 *
 * Arms rotation priority (Turtle WoW 1.18.1):
 * 1. Execute (below 20%)
 * 2. Mortal Strike (if talented)
 * 3. Overpower (on dodge/parry proc)
 * 4. Heroic Strike (when rage >= 20)
 * 5. Rend (maintain debuff)
 * 6. Thunder Clap (maintain debuff)
 * 7. Auto-attack (default)
 *
 * Stance: Battle Stance preferred
 */
#ifndef _PLAYERBOT_ARMS_WARRIOR_STRATEGY_H
#define _PLAYERBOT_ARMS_WARRIOR_STRATEGY_H

#include "Strategy/GenericWarriorStrategy.h"

class PlayerBotAI;

class ArmsWarriorStrategy : public GenericWarriorStrategy
{
public:
    ArmsWarriorStrategy(PlayerBotAI* botAI);
    virtual ~ArmsWarriorStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;

protected:
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const override;
};

#endif // _PLAYERBOT_ARMS_WARRIOR_STRATEGY_H
