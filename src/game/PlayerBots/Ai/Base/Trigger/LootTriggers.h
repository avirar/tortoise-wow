#ifndef _PLAYERBOT_LOOTTRIGGERS_H
#define _PLAYERBOT_LOOTTRIGGERS_H

#include "Trigger.h"

class PlayerBotAI;

class LootAvailableTrigger : public Trigger
{
public:
    LootAvailableTrigger(PlayerBotAI* botAI) : Trigger(botAI, "loot available") {}
    bool IsActive() override;
};

class FarFromLootTrigger : public Trigger
{
public:
    FarFromLootTrigger(PlayerBotAI* botAI) : Trigger(botAI, "far from loot target") {}
    bool IsActive() override;
};

class CanLootTrigger : public Trigger
{
public:
    CanLootTrigger(PlayerBotAI* botAI) : Trigger(botAI, "can loot") {}
    bool IsActive() override;
};

class LootOpenTrigger : public Trigger
{
public:
    LootOpenTrigger(PlayerBotAI* botAI) : Trigger(botAI, "loot open") {}
    bool IsActive() override;
};

#endif
