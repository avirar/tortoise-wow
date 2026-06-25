#include "AvailableLootValue.h"
#include "PlayerBotAI.h"
#include "AiObjectContext.h"
#include "Value.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "SharedDefines.h"

AvailableLootValue::AvailableLootValue(PlayerBotAI* botAI, std::string const name)
    : ManualSetValue<LootObjectStack*>(botAI, nullptr, name)
{
    value = new LootObjectStack(bot);
}

AvailableLootValue::~AvailableLootValue()
{
    delete value;
}

LootTargetValue::LootTargetValue(PlayerBotAI* botAI, std::string const name)
    : ManualSetValue<LootObject>(botAI, LootObject(), name)
{
}

bool HasAvailableLootValue::Calculate()
{
    LootObjectStack* stack = GetAiObjectContext()->GetValue<LootObjectStack*>("available loot")->Get();
    if (!stack)
        return false;
    return stack->CanLoot(INTERACTION_DISTANCE);
}

bool CanLootValue::Calculate()
{
    LootObject loot = GetAiObjectContext()->GetValue<LootObject>("loot target")->Get();
    if (loot.IsEmpty())
        return false;

    Map* map = bot->GetMap();
    Creature* creature = map->GetCreature(loot.guid);
    GameObject* go = map->GetGameObject(loot.guid);

    WorldObject* wo = creature ? static_cast<WorldObject*>(creature) :
                                go ? static_cast<WorldObject*>(go) : nullptr;
    if (!wo)
        return false;

    return bot->GetDistance2d(wo) <= INTERACTION_DISTANCE - 2.0f;
}

uint8 BagSpaceValue::Calculate()
{
    uint32 totalSlots = 0;
    uint32 usedSlots = 0;

    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        Item* bagItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!bagItem)
            continue;

        Bag* bag = bagItem->ToBag();
        if (!bag)
            continue;

        totalSlots += bag->GetBagSize();
        for (uint8 j = 0; j < bag->GetBagSize(); ++j)
        {
            if (bot->GetItemByPos(i, j))
                ++usedSlots;
        }
    }

    totalSlots += INVENTORY_SLOT_BAG_START;
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            ++usedSlots;
    }

    if (totalSlots == 0)
        return 100;

    return uint8((usedSlots * 100) / totalSlots);
}
