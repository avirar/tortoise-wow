#include "LootStrategyValue.h"

LootStrategy* LootStrategyValue::normal_ = new NormalLootStrategy();
LootStrategy* LootStrategyValue::all_ = new AllLootStrategy();

bool LootStrategyValue::Load(std::string const text)
{
    value = LootStrategyValue::instance(text);
    return true;
}

LootStrategy* LootStrategyValue::instance(std::string const strategy)
{
    if (strategy == "*" || strategy == "all")
        return all_;
    return normal_;
}
