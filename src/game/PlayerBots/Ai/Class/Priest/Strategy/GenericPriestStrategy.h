/**
 * @file GenericPriestStrategy.h
 * @brief Base priest combat strategy (common to all specs)
 *
 * AC mod-playerbots pattern. The class offensive table
 * (PlayerBotAI::SelectOffensiveSpell) resolves Shadow Word: Pain/Mind
 * Blast/Smite for CLASS_PRIEST, so "cast spell" is the nuke. We add
 * combat self-heal (survivability) on top.
 */
#ifndef _PLAYERBOT_GENERIC_PRIEST_STRATEGY_H
#define _PLAYERBOT_GENERIC_PRIEST_STRATEGY_H

#include "../../Base/Strategy/RangedCombatStrategy.h"

class PlayerBotAI;

class GenericPriestStrategy : public RangedCombatStrategy
{
public:
    GenericPriestStrategy(PlayerBotAI* botAI);
    virtual ~GenericPriestStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::vector<NextAction> getDefaultActions() override;

protected:
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) {}
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const { return {}; }
};

#endif // _PLAYERBOT_GENERIC_PRIEST_STRATEGY_H
