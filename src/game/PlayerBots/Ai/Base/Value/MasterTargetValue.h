#ifndef PLAYERBOT_MASTER_TARGET_VALUE_H
#define PLAYERBOT_MASTER_TARGET_VALUE_H

#include "Value/Value.h"

class PlayerBotAI;

class MasterTargetValue : public UnitCalculatedValue
{
public:
    MasterTargetValue(PlayerBotAI* botAI);
    Unit* Calculate() override;
};

#endif
