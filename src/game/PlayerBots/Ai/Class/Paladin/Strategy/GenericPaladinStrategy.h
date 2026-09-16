/**
 * @file GenericPaladinStrategy.h
 * @brief Base paladin combat strategy (common to all specs)
 *
 * AC mod-playerbots pattern. The class offensive table
 * (PlayerBotAI::SelectOffensiveSpell) resolves Judgement/Exorcism for
 * CLASS_PALADIN, so "cast spell" is the nuke. We add Hammer of Justice
 * (melee stun) and combat self-heal on top.
 */
#ifndef _PLAYERBOT_GENERIC_PALADIN_STRATEGY_H
#define _PLAYERBOT_GENERIC_PALADIN_STRATEGY_H

#include "../../Base/Strategy/MeleeCombatStrategy.h"

class PlayerBotAI;

class GenericPaladinStrategy : public MeleeCombatStrategy
{
public:
    GenericPaladinStrategy(PlayerBotAI* botAI);
    virtual ~GenericPaladinStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::vector<NextAction> getDefaultActions() override;

protected:
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) {}
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const { return {}; }
};

#endif // _PLAYERBOT_GENERIC_PALADIN_STRATEGY_H
