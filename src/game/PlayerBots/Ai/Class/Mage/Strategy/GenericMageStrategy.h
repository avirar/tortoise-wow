/**
 * @file GenericMageStrategy.h
 * @brief Base mage combat strategy (common to all specs)
 *
 * AC mod-playerbots pattern: a base class strategy with a named-action
 * default list and trigger-driven specializations. Vanilla 1.12 mages
 * share the same low-level toolkit (Firebolt/Frostbolt/Arcane Explosion
 * via the class offensive table + Polymorph CC), so spec branches
 * (Arcane/Fire/Frost) are added as levels/talents unlock.
 */
#ifndef _PLAYERBOT_GENERIC_MAGE_STRATEGY_H
#define _PLAYERBOT_GENERIC_MAGE_STRATEGY_H

#include "../../Base/Strategy/RangedCombatStrategy.h"

class PlayerBotAI;

class GenericMageStrategy : public RangedCombatStrategy
{
public:
    GenericMageStrategy(PlayerBotAI* botAI);
    virtual ~GenericMageStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::vector<NextAction> getDefaultActions() override;

protected:
    // Spec override hook: child classes add their own triggers here
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) {}

    // Spec override hook: child classes define their default action priorities
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const { return {}; }
};

#endif // _PLAYERBOT_GENERIC_MAGE_STRATEGY_H
