#ifndef _PLAYERBOT_LOOTSTRATEGYVALUE_H
#define _PLAYERBOT_LOOTSTRATEGYVALUE_H

#include "Value.h"

class PlayerBotAI;

struct ItemPrototype;

class LootStrategy
{
public:
    LootStrategy() {}
    virtual ~LootStrategy() {}
    virtual bool CanLoot(ItemPrototype const* proto) = 0;
    virtual std::string const GetName() = 0;
};

class NormalLootStrategy : public LootStrategy
{
public:
    bool CanLoot(ItemPrototype const* proto) override
    {
        if (!proto)
            return false;
        if (proto->Quality == ITEM_QUALITY_POOR)
            return false;
        return true;
    }
    std::string const GetName() override { return "normal"; }
};

class AllLootStrategy : public LootStrategy
{
public:
    bool CanLoot(ItemPrototype const* /*proto*/) override { return true; }
    std::string const GetName() override { return "all"; }
};

class LootStrategyValue : public ManualSetValue<LootStrategy*>
{
public:
    LootStrategyValue(PlayerBotAI* botAI, std::string const name = "loot strategy")
        : ManualSetValue<LootStrategy*>(botAI, all_, name) {}  // AC: bots loot everything by default
    virtual ~LootStrategyValue() {}

    std::string const Save() override { return value ? value->GetName() : "?"; }
    bool Load(std::string const value) override;

    static LootStrategy* normal_;
    static LootStrategy* all_;
    static LootStrategy* instance(std::string const name);
};

#endif
