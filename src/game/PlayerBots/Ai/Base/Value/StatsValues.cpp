#include "StatsValues.h"

#include "PlayerBotAI.h"
#include "Player.h"

uint8 HealthValue::Calculate()
{
    if (!bot)
        return 100;

    return static_cast<uint8>((static_cast<float>(bot->GetHealth()) / bot->GetMaxHealth()) * 100);
}

uint8 ManaValue::Calculate()
{
    if (!bot)
        return 100;

    return static_cast<uint8>((static_cast<float>(bot->GetPower(POWER_MANA)) / bot->GetMaxPower(POWER_MANA)) * 100);
}

bool HasManaValue::Calculate()
{
    if (!bot)
        return false;

    return bot->GetPowerType() == POWER_MANA;
}

bool IsDeadValue::Calculate()
{
    if (!bot)
        return false;

    return !bot->IsAlive();
}
