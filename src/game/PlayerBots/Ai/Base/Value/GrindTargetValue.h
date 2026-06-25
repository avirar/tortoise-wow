#ifndef PLAYERBOT_GRIND_TARGET_VALUE_H
#define PLAYERBOT_GRIND_TARGET_VALUE_H

#include "Value/Value.h"

class PlayerBotAI;

class GrindTargetValue : public UnitCalculatedValue
{
public:
    GrindTargetValue(PlayerBotAI* botAI);
    Unit* Calculate() override;
};

#endif
