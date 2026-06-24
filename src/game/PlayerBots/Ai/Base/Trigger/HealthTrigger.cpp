#include "HealthTrigger.h"

#include "Player.h"
#include "SharedDefines.h"
#include "PlayerbotAIConfig.h"

LowHealthTrigger::LowHealthTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "low health")
{
}

bool LowHealthTrigger::IsActive()
{
    return bot->GetHealthPercent() <= sPlayerbotAIConfig.lowHealth;
}

MediumHealthTrigger::MediumHealthTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "medium health")
{
}

bool MediumHealthTrigger::IsActive()
{
    return bot->GetHealthPercent() <= sPlayerbotAIConfig.mediumHealth;
}

LowManaTrigger::LowManaTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "low mana")
{
}

bool LowManaTrigger::IsActive()
{
    return bot->GetPowerPercent(POWER_MANA) <= sPlayerbotAIConfig.lowMana && bot->GetPowerType() == POWER_MANA;
}

HighManaTrigger::HighManaTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "high mana")
{
}

bool HighManaTrigger::IsActive()
{
    return bot->GetPowerPercent(POWER_MANA) >= sPlayerbotAIConfig.highMana && bot->GetPowerType() == POWER_MANA;
}
