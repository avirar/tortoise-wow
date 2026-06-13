#ifndef _PLAYERBOT_CUSTOM_STRATEGY_H
#define _PLAYERBOT_CUSTOM_STRATEGY_H

#include <vector>
#include <string>
#include "Strategy/Strategy.h"

class PlayerbotAI;

class CustomStrategy : public Strategy
{
public:
    CustomStrategy(PlayerBotAI* botAI);
    virtual ~CustomStrategy() {}

    virtual std::string const getName() { return "custom"; }
    virtual uint32 GetType() const { return STRATEGY_TYPE_GENERIC; }

    virtual std::vector<NextAction> getDefaultActions();
};

#endif
