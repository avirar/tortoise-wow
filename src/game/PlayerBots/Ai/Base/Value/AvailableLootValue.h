#ifndef _PLAYERBOT_AVAILABLELOOTVALUE_H
#define _PLAYERBOT_AVAILABLELOOTVALUE_H

#include "Value.h"
#include "Mgr/Item/LootObjectStack.h"

class PlayerBotAI;

class AvailableLootValue : public ManualSetValue<LootObjectStack*>
{
public:
    AvailableLootValue(PlayerBotAI* botAI, std::string const name = "available loot");
    virtual ~AvailableLootValue();
};

class LootTargetValue : public ManualSetValue<LootObject>
{
public:
    LootTargetValue(PlayerBotAI* botAI, std::string const name = "loot target");
};

class HasAvailableLootValue : public BoolCalculatedValue
{
public:
    HasAvailableLootValue(PlayerBotAI* botAI, std::string const name = "has available loot")
        : BoolCalculatedValue(botAI, name) {}

    bool Calculate() override;
};

class CanLootValue : public BoolCalculatedValue
{
public:
    CanLootValue(PlayerBotAI* botAI, std::string const name = "can loot")
        : BoolCalculatedValue(botAI, name) {}

    bool Calculate() override;
};

class BagSpaceValue : public Uint8CalculatedValue
{
public:
    BagSpaceValue(PlayerBotAI* botAI, std::string const name = "bag space")
        : Uint8CalculatedValue(botAI, name) {}

    uint8 Calculate() override;
};

#endif
