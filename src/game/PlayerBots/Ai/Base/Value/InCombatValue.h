#ifndef PLAYERBOT_INCOMBAT_VALUE_H
#define PLAYERBOT_INCOMBAT_VALUE_H

#include "Value.h"

class PlayerBotAI;

class InCombatValue : public BoolCalculatedValue
{
public:
    InCombatValue(PlayerBotAI* botAI);
    virtual bool Calculate() override;
};

#endif
