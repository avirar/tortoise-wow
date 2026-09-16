/**
 * @file GenericShamanStrategy.h
 * @brief Base shaman combat strategy (common to all specs)
 *
 * AC mod-playerbots pattern. The class offensive table
 * (PlayerBotAI::SelectOffensiveSpell) resolves Flame Shock/Earth Shock/
 * Lightning Bolt for CLASS_SHAMAN, so "cast spell" is the nuke. We add
 * combat self-heal (survivability) on top.
 */
#ifndef _PLAYERBOT_GENERIC_SHAMAN_STRATEGY_H
#define _PLAYERBOT_GENERIC_SHAMAN_STRATEGY_H

#include "../../Base/Strategy/RangedCombatStrategy.h"

class PlayerBotAI;

class GenericShamanStrategy : public RangedCombatStrategy
{
public:
    GenericShamanStrategy(PlayerBotAI* botAI);
    virtual ~GenericShamanStrategy() {}

    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::vector<NextAction> getDefaultActions() override;

protected:
    virtual void InitSpecTriggers(std::vector<TriggerNode*>& triggers) {}
    virtual std::vector<std::pair<std::string, float>> GetSpecDefaultActions() const { return {}; }
};

#endif // _PLAYERBOT_GENERIC_SHAMAN_STRATEGY_H
