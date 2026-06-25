#include "ItemUsageValue.h"
#include "PlayerBotAI.h"
#include "Player.h"
#include "Item.h"
#include "ItemPrototype.h"
#include "ObjectMgr.h"
#include "AiObjectContext.h"
#include "SharedDefines.h"

ItemUsage ItemUsageValue::Calculate()
{
    return ITEM_USAGE_NONE;
}

ItemUsage ItemUsageValue::QueryItemUsageForEquip(ItemPrototype const* proto)
{
    if (!proto || proto->InventoryType == INVTYPE_NON_EQUIP)
        return ITEM_USAGE_NONE;

    if (proto->Class != ITEM_CLASS_ARMOR && proto->Class != ITEM_CLASS_WEAPON &&
        proto->Class != ITEM_CLASS_CONTAINER)
        return ITEM_USAGE_NONE;

    if (bot->CanUseItem(proto) != EQUIP_ERR_OK)
        return ITEM_USAGE_NONE;

    uint8 dstSlot = NULL_SLOT;
    switch (proto->InventoryType)
    {
        case INVTYPE_HEAD: dstSlot = EQUIPMENT_SLOT_HEAD; break;
        case INVTYPE_NECK: dstSlot = EQUIPMENT_SLOT_NECK; break;
        case INVTYPE_CLOAK: dstSlot = EQUIPMENT_SLOT_BACK; break;
        case INVTYPE_CHEST: dstSlot = EQUIPMENT_SLOT_CHEST; break;
        case INVTYPE_WAIST: dstSlot = EQUIPMENT_SLOT_WAIST; break;
        case INVTYPE_LEGS: dstSlot = EQUIPMENT_SLOT_LEGS; break;
        case INVTYPE_FEET: dstSlot = EQUIPMENT_SLOT_FEET; break;
        case INVTYPE_WRISTS: dstSlot = EQUIPMENT_SLOT_WRISTS; break;
        case INVTYPE_HANDS: dstSlot = EQUIPMENT_SLOT_HANDS; break;
        case INVTYPE_FINGER: dstSlot = EQUIPMENT_SLOT_FINGER1; break;
        case INVTYPE_TRINKET: dstSlot = EQUIPMENT_SLOT_FINGER2; break;
        case INVTYPE_WEAPONMAINHAND: dstSlot = EQUIPMENT_SLOT_MAINHAND; break;
        case INVTYPE_WEAPONOFFHAND: dstSlot = EQUIPMENT_SLOT_OFFHAND; break;
        case INVTYPE_RANGED: dstSlot = EQUIPMENT_SLOT_RANGED; break;
        case INVTYPE_2HWEAPON: dstSlot = EQUIPMENT_SLOT_MAINHAND; break;
        default: return ITEM_USAGE_NONE;
    }

    if (dstSlot == NULL_SLOT)
        return ITEM_USAGE_NONE;

    float itemScore = proto->ItemLevel * (proto->Quality + 1);

    if (itemScore <= 0.0f)
        return ITEM_USAGE_NONE;

    Item* oldItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, dstSlot);
    if (!oldItem)
    {
        if (itemScore > 0.0f)
            return ITEM_USAGE_EQUIP;
        return ITEM_USAGE_NONE;
    }

    ItemPrototype const* oldProto = oldItem->GetProto();
    float oldScore = oldProto->ItemLevel * (oldProto->Quality + 1);

    if (itemScore > oldScore * 1.1f)
    {
        return ITEM_USAGE_REPLACE;
    }

    return ITEM_USAGE_NONE;
}

ItemUsage ItemUpgradeValue::Calculate()
{
    return ITEM_USAGE_NONE;
}
