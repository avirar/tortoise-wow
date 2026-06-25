#ifndef PLAYERBOT_STATS_VALUES_H
#define PLAYERBOT_STATS_VALUES_H

#include "Value/Value.h"

class PlayerBotAI;
class Unit;

class HealthValue : public Uint8CalculatedValue
{
public:
    HealthValue(PlayerBotAI* botAI) : Uint8CalculatedValue(botAI, "health") {}
    uint8 Calculate() override;
};

class ManaValue : public Uint8CalculatedValue
{
public:
    ManaValue(PlayerBotAI* botAI) : Uint8CalculatedValue(botAI, "mana") {}
    uint8 Calculate() override;
};

class HasManaValue : public BoolCalculatedValue
{
public:
    HasManaValue(PlayerBotAI* botAI) : BoolCalculatedValue(botAI, "has mana", 2000) {}
    bool Calculate() override;
};

class IsDeadValue : public BoolCalculatedValue
{
public:
    IsDeadValue(PlayerBotAI* botAI) : BoolCalculatedValue(botAI, "dead") {}
    bool Calculate() override;
};

#endif
