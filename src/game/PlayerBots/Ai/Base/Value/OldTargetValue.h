#ifndef PLAYERBOT_OLDTARGETVALUE_H
#define PLAYERBOT_OLDTARGETVALUE_H

#include "Value/Value.h"

class PlayerBotAI;

class OldTargetValue : public UnitManualSetValue
{
public:
    OldTargetValue(PlayerBotAI* botAI)
        : UnitManualSetValue(botAI, nullptr, "old target")
    {
    }
};

#endif
