/*
 * Ported from AzerothCore mod-playerbots StrategyContext pattern.
 */

#include "StrategyContext.h"
#include "PlayerBotAI.h"
#include "Strategy/NonCombatStrategy.h"
#include "Strategy/CombatStrategy.h"
#include "Strategy/MeleeCombatStrategy.h"
#include "Strategy/RangedCombatStrategy.h"
#include "Strategy/WanderStrategy.h"
#include "Strategy/GrindingStrategy.h"
#include "Strategy/DpsAssistStrategy.h"
#include "Strategy/LootNonCombatStrategy.h"
#include "Strategy/DeadStrategy.h"
#include "Strategy/UseFoodStrategy.h"

Strategy* StrategyContext::non_combat(PlayerBotAI* botAI) { return new NonCombatStrategy(botAI); }
Strategy* StrategyContext::combat(PlayerBotAI* botAI) { return new CombatStrategy(botAI); }
Strategy* StrategyContext::melee(PlayerBotAI* botAI) { return new MeleeCombatStrategy(botAI); }
Strategy* StrategyContext::ranged(PlayerBotAI* botAI) { return new RangedCombatStrategy(botAI); }
Strategy* StrategyContext::wander(PlayerBotAI* botAI) { return new WanderStrategy(botAI); }
Strategy* StrategyContext::grind(PlayerBotAI* botAI) { return new GrindingStrategy(botAI); }
Strategy* StrategyContext::dps_assist(PlayerBotAI* botAI) { return new DpsAssistStrategy(botAI); }
Strategy* StrategyContext::loot(PlayerBotAI* botAI) { return new LootNonCombatStrategy(botAI); }
Strategy* StrategyContext::dead(PlayerBotAI* botAI) { return new DeadStrategy(botAI); }
Strategy* StrategyContext::food(PlayerBotAI* botAI) { return new UseFoodStrategy(botAI); }
