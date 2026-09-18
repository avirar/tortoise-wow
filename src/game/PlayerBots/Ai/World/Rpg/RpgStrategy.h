/*
 * Rpg strategy — R7 L2. Port of AC `NewRpgStrategy`
 * (src/Ai/World/Rpg/Strategy/NewRpgStrategy.h/.cpp).
 *
 * getDefaultActions: "rpg status update" @ relevance 11.0 (the state-machine
 * driver — runs every tick; "the relevance should be greater than grind" per AC's
 * comment). InitTriggers: one status trigger per active status, each firing the
 * status's movement action @ relevance 3.0.
 *
 * Vanilla v1: only GO_GRIND + WANDER_RANDOM have movement actions (the bot sits
 * while REST, idles while IDLE). DO_QUEST/GoCamp/TravelFlight/etc. are the WotLK
 * statuses omitted for this core (see R7 in bot-master-plan.md).
 */
#ifndef _PLAYERBOT_RPG_STRATEGY_H
#define _PLAYERBOT_RPG_STRATEGY_H

#include "Strategy.h"

class PlayerBotAI;

class RpgStrategy : public Strategy
{
public:
    RpgStrategy(PlayerBotAI* botAI) : Strategy(botAI) {}
    virtual ~RpgStrategy() {}

    virtual std::vector<NextAction> getDefaultActions() override;
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
    virtual std::string const getName() { return "rpg"; }
};

#endif
