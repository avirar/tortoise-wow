#ifndef PLAYERBOT_DPS_ASSIST_STRATEGY_H
#define PLAYERBOT_DPS_ASSIST_STRATEGY_H

#include "Strategy.h"

class PlayerBotAI;

class DpsAssistStrategy : public Strategy
{
public:
    DpsAssistStrategy(PlayerBotAI* botAI);
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "dps assist"; }
};

#endif
