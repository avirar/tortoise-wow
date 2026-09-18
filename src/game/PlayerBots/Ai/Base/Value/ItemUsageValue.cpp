#include "ItemUsageValue.h"
#include "PlayerBotAI.h"
#include "Player.h"
#include "Item.h"
#include "ItemPrototype.h"
#include "ObjectMgr.h"
#include "AiObjectContext.h"
#include "SharedDefines.h"
#include "Logging.h"
#include "Log.h"
#include "Mgr/Item/StatsWeightCalculator.h"
#include "PlayerbotAIConfig.h"

ItemUsage ItemUsageValue::Calculate()
{
    return ITEM_USAGE_NONE;
}

ItemUsage ItemUsageValue::QueryItemUsageForEquip(ItemPrototype const* proto)
{
    LOG_DEBUG("playerbots", "ItemUsage: %s QueryItemUsageForEquip called for %u class=%u invType=%u",
        bot->GetName(), proto->ItemId, proto->Class, proto->InventoryType);

    if (!proto || proto->InventoryType == INVTYPE_NON_EQUIP)
        return ITEM_USAGE_NONE;

    if (proto->Class != ITEM_CLASS_ARMOR && proto->Class != ITEM_CLASS_WEAPON &&
        proto->Class != ITEM_CLASS_CONTAINER)
    {
        LOG_DEBUG("playerbots", "ItemUsage: %s class=%u for %u (not armor/weapon/container)",
            bot->GetName(), proto->Class, proto->ItemId);
        return ITEM_USAGE_NONE;
    }

    InventoryResult canUse = bot->CanUseItem(proto);
    if (canUse != EQUIP_ERR_OK)
    {
        LOG_DEBUG("playerbots", "ItemUsage: %s CanUseItem failed for %u '%s' result=%d",
            bot->GetName(), proto->ItemId, proto->Name1.c_str(), canUse);
        return ITEM_USAGE_NONE;
    }

    uint8 dstSlot = NULL_SLOT;
    switch (proto->InventoryType)
    {
        case INVTYPE_HEAD: dstSlot = EQUIPMENT_SLOT_HEAD; break;
        case INVTYPE_NECK: dstSlot = EQUIPMENT_SLOT_NECK; break;
        case INVTYPE_SHOULDERS: dstSlot = EQUIPMENT_SLOT_SHOULDERS; break;
        case INVTYPE_BODY: dstSlot = EQUIPMENT_SLOT_BODY; break;
        case INVTYPE_CLOAK: dstSlot = EQUIPMENT_SLOT_BACK; break;
        case INVTYPE_CHEST: dstSlot = EQUIPMENT_SLOT_CHEST; break;
        case INVTYPE_WAIST: dstSlot = EQUIPMENT_SLOT_WAIST; break;
        case INVTYPE_LEGS: dstSlot = EQUIPMENT_SLOT_LEGS; break;
        case INVTYPE_FEET: dstSlot = EQUIPMENT_SLOT_FEET; break;
        case INVTYPE_WRISTS: dstSlot = EQUIPMENT_SLOT_WRISTS; break;
        case INVTYPE_HANDS: dstSlot = EQUIPMENT_SLOT_HANDS; break;
        case INVTYPE_FINGER: dstSlot = EQUIPMENT_SLOT_FINGER1; break;
        case INVTYPE_TRINKET: dstSlot = EQUIPMENT_SLOT_TRINKET1; break;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_2HWEAPON: dstSlot = EQUIPMENT_SLOT_MAINHAND; break;
        case INVTYPE_SHIELD:
        case INVTYPE_WEAPONOFFHAND: dstSlot = EQUIPMENT_SLOT_OFFHAND; break;
        case INVTYPE_RANGED: dstSlot = EQUIPMENT_SLOT_RANGED; break;
        default: return ITEM_USAGE_NONE;
    }

    if (dstSlot == NULL_SLOT)
        return ITEM_USAGE_NONE;

    // AC pattern: use StatsWeightCalculator for scoring (not crude ilvl*quality)
    StatsWeightCalculator calculator(bot);
    float itemScore = calculator.CalculateItem(proto->ItemId);

    if (itemScore <= 0.0f)
    {
        LOG_DEBUG("playerbots", "ItemUsage: %s score=%.2f for %u '%s' ilvl=%u quality=%u",
            bot->GetName(), itemScore, proto->ItemId, proto->Name1.c_str(), proto->ItemLevel, proto->Quality);
        return ITEM_USAGE_NONE;
    }

    Item* oldItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, dstSlot);
    if (!oldItem)
    {
        if (itemScore > 0.0f)
        {
            LOG_DEBUG("playerbots", "ItemUsage: %s EQUIP %u score=%.2f (empty slot %u)",
                bot->GetName(), proto->ItemId, itemScore, dstSlot);
            return ITEM_USAGE_EQUIP;
        }
        return ITEM_USAGE_NONE;
    }

    ItemPrototype const* oldProto = oldItem->GetProto();
    float oldScore = calculator.CalculateItem(oldProto->ItemId);

    // AC pattern: equipUpgradeThreshold default 1.1 (10% improvement needed)
    float threshold = sPlayerbotAIConfig.equipUpgradeThreshold;
    LOG_DEBUG("playerbots", "ItemUsage: %s compare %u new=%.2f vs %u old=%.2f threshold=%.2f",
        bot->GetName(), proto->ItemId, itemScore, oldProto->ItemId, oldScore, threshold);
    if (itemScore > oldScore * threshold)
    {
        LOG_DEBUG("playerbots", "ItemUsage: %s REPLACE %u (upgrade)", bot->GetName(), proto->ItemId);
        return ITEM_USAGE_REPLACE;
    }

    LOG_DEBUG("playerbots", "ItemUsage: %s NONE %u (no upgrade)", bot->GetName(), proto->ItemId);
    return ITEM_USAGE_NONE;
}

ItemUsage ItemUpgradeValue::Calculate()
{
    return ITEM_USAGE_NONE;
}
