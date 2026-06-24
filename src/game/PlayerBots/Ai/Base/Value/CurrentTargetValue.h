#ifndef PLAYERBOT_CURRENT_TARGET_VALUE_H
#define PLAYERBOT_CURRENT_TARGET_VALUE_H

#include "Value/Value.h"

class PlayerBotAI;

class CurrentTargetValue : public UnitManualSetValue
{
public:
    CurrentTargetValue(PlayerBotAI* botAI);
};

#endif
