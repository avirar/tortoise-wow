/**
 * @file GenericDruidStrategy.h
 * @brief Base druid combat strategy (common to all specs)
 *
 * AC mod-playerbots pattern. The class offensive table
 * (PlayerBotAI::SelectOffensiveSpell) resolves Rake/Shred/Wrath/Moonfire/
 * Entangling Roots for CLASS_DRUID, so "cast spell" is the nuke (with
 * melee auto-attack fallback). We add combat self-heal (survivability)
 * on top.
 */
#ifndef _PLAYERBOT_GENERIC_DRUID_STRATEGY_H
#define _PLAYERBOT_GENERIC_DRUID_STRATEGY_H

#include "../../Base/Strategy/MeleeCombatStrategy.h"

class PlayerBotAI;

class GenericDruidStrategy : public MeleeCombatStrategy
{
public:
    GenericDruidStrategy(PlayerBotAI* botAI);
    virtual ~GenericDruidStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::vector<NextAction> getDefaultActions() override;

protected:
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) {}
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const { return {}; }
};

#endif // _PLAYERBOT_GENERIC_DRUID_STRATEGY_H
