/**
 * @file GenericWarriorStrategy.h
 * @brief Base warrior combat strategy (common to all specs)
 *
 * Handles stance management, buff rotation, debuff maintenance, and
 * common warrior abilities. Specialized strategies (Arms/Fury/Prot)
 * extend this to add spec-specific rotation priorities.
 */
#ifndef _PLAYERBOT_GENERIC_WARRIOR_STRATEGY_H
#define _PLAYERBOT_GENERIC_WARRIOR_STRATEGY_H

#include "Strategy/CombatStrategy.h"

class PlayerBotAI;

class GenericWarriorStrategy : public CombatStrategy
{
public:
    GenericWarriorStrategy(PlayerBotAI* botAI);
    virtual ~GenericWarriorStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;

protected:
    // Spec override hook: child classes add their own triggers here
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) {}

    // Spec override hook: child classes define their default action priorities
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const { return {}; }
};

#endif // _PLAYERBOT_GENERIC_WARRIOR_STRATEGY_H
