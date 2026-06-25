#ifndef _PLAYERBOT_WAITFORATTACKSTRATEGY_H
#define _PLAYERBOT_WAITFORATTACKSTRATEGY_H

#include "Strategy.h"

class PlayerBotAI;

class WaitForAttackStrategy : public Strategy
{
public:
    WaitForAttackStrategy(PlayerBotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "wait for attack"; }

    // AC pattern: ShouldWait returns true only in group with real player master
    // For solo bots, always returns false (attack immediately)
    static bool ShouldWait(PlayerBotAI* botAI);

    static float GetSafeDistance() { return 40.0f; }
    static float GetSafeDistanceThreshold() { return 2.5f; }

private:
    void InitTriggers(std::vector<TriggerNode*>& triggers) override {}
};

#endif
