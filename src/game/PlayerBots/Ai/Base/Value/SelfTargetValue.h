#ifndef PLAYERBOT_SELF_TARGET_VALUE_H
#define PLAYERBOT_SELF_TARGET_VALUE_H

#include "Value/Value.h"

class PlayerBotAI;

class SelfTargetValue : public UnitCalculatedValue
{
public:
    SelfTargetValue(PlayerBotAI* botAI) : UnitCalculatedValue(botAI, "self target") {}
    Unit* Calculate() override;
};

#endif
