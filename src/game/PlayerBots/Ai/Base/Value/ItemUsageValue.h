#ifndef _PLAYERBOT_ITEMUSAGEVALUE_H
#define _PLAYERBOT_ITEMUSAGEVALUE_H

#include "Value.h"

class Item;
class Player;
class PlayerBotAI;

struct ItemPrototype;

enum ItemUsage : uint32
{
    ITEM_USAGE_NONE = 0,
    ITEM_USAGE_EQUIP = 1,
    ITEM_USAGE_REPLACE = 2,
    ITEM_USAGE_BAD_EQUIP = 3,
    ITEM_USAGE_VENDOR = 4,
    ITEM_USAGE_KEEP = 5,
};

class ItemUsageValue : public CalculatedValue<ItemUsage>
{
public:
    ItemUsageValue(PlayerBotAI* botAI, std::string const name = "item usage")
        : CalculatedValue<ItemUsage>(botAI, name) {}

    ItemUsage Calculate() override;
    ItemUsage QueryItemUsageForEquip(ItemPrototype const* proto);
};

class ItemUpgradeValue : public ItemUsageValue
{
public:
    ItemUpgradeValue(PlayerBotAI* botAI, std::string const name = "item upgrade")
        : ItemUsageValue(botAI, name) {}

    ItemUsage Calculate() override;
};

#endif
