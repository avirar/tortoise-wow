/*
 * Ported from AzerothCore mod-playerbots StrategyContext.h
 * Strategy factory pattern: named creators for Strategy* instantiation.
 */

#ifndef _PLAYERBOT_STRATEGYCONTEXT_H
#define _PLAYERBOT_STRATEGYCONTEXT_H

#include "NamedObjectContext.h"
#include "Strategy/Strategy.h"

// Forward declarations
class NonCombatStrategy;
class CombatStrategy;
class MeleeCombatStrategy;
class RangedCombatStrategy;
class WanderStrategy;
class GrindingStrategy;
class DpsAssistStrategy;
class LootNonCombatStrategy;
class DeadStrategy;
class UseFoodStrategy;

class StrategyContext : public NamedObjectContext<Strategy>
{
public:
    StrategyContext()
    {
        creators["nc"] = &StrategyContext::non_combat;
        creators["combat"] = &StrategyContext::combat;
        creators["melee"] = &StrategyContext::melee;
        creators["ranged"] = &StrategyContext::ranged;
        creators["wander"] = &StrategyContext::wander;
        creators["grind"] = &StrategyContext::grind;
        creators["dps assist"] = &StrategyContext::dps_assist;
        creators["loot"] = &StrategyContext::loot;
        creators["dead"] = &StrategyContext::dead;
        creators["food"] = &StrategyContext::food;
    }

private:
    static Strategy* non_combat(PlayerBotAI* botAI);
    static Strategy* combat(PlayerBotAI* botAI);
    static Strategy* melee(PlayerBotAI* botAI);
    static Strategy* ranged(PlayerBotAI* botAI);
    static Strategy* wander(PlayerBotAI* botAI);
    static Strategy* grind(PlayerBotAI* botAI);
    static Strategy* dps_assist(PlayerBotAI* botAI);
    static Strategy* loot(PlayerBotAI* botAI);
    static Strategy* dead(PlayerBotAI* botAI);
    static Strategy* food(PlayerBotAI* botAI);
};

#endif
