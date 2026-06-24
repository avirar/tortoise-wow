#ifndef PLAYERBOT_DPS_TARGET_VALUE_H
#define PLAYERBOT_DPS_TARGET_VALUE_H

#include "Value/Value.h"

class PlayerBotAI;

class DpsTargetValue : public UnitCalculatedValue
{
public:
    DpsTargetValue(PlayerBotAI* botAI);
    Unit* Calculate() override;
};

#endif
