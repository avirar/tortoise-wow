/**
 * @file GenericWarlockStrategy.h
 * @brief Base warlock combat strategy (common to all specs)
 *
 * AC mod-playerbots pattern. The class offensive table
 * (PlayerBotAI::SelectOffensiveSpell) already resolves Shadow Bolt/
 * Corruption/Immolate/Curse of Agony/Drain Life for CLASS_WARLOCK, so
 * "cast spell" is the nuke. We add Fear CC on top (the warlock's learnable
 * low-level crowd control).
 */
#ifndef _PLAYERBOT_GENERIC_WARLOCK_STRATEGY_H
#define _PLAYERBOT_GENERIC_WARLOCK_STRATEGY_H

#include "../../Base/Strategy/RangedCombatStrategy.h"

class PlayerBotAI;

class GenericWarlockStrategy : public RangedCombatStrategy
{
public:
    GenericWarlockStrategy(PlayerBotAI* botAI);
    virtual ~GenericWarlockStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::vector<NextAction> getDefaultActions() override;

protected:
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) {}
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const { return {}; }
};

#endif // _PLAYERBOT_GENERIC_WARLOCK_STRATEGY_H
