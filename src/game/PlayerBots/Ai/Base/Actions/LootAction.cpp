#include "LootAction.h"
#include "PlayerBotAI.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "LootMgr.h"
#include "Mgr/Item/LootObjectStack.h"
#include "Value/LootStrategyValue.h"
#include "Value/ItemUsageValue.h"
#include "Value/AvailableLootValue.h"
#include "AiObjectContext.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "SharedDefines.h"
#include "Logging.h"
#include "Log.h"
#include "Opcodes.h"
#include "CellImpl.h"
#include <sstream>

bool LootAction::Execute([[maybe_unused]] Event event)
{
    LootObjectStack* stack = GetAiObjectContext()->GetValue<LootObjectStack*>("available loot")->Get();
    if (!stack)
        return false;

    LootObject lootObject = stack->GetLoot(INTERACTION_DISTANCE);
    if (lootObject.IsEmpty())
        return false;

    GetAiObjectContext()->GetValue<LootObject>("loot target")->Set(lootObject);
    return true;
}

bool LootAction::isUseful()
{
    if (bot->GetGroup() && bot->GetGroup()->GetLootMethod() == FREE_FOR_ALL)
        return false;
    return true;
}

bool OpenLootAction::Execute([[maybe_unused]] Event event)
{
    LootObject lootObject = GetAiObjectContext()->GetValue<LootObject>("loot target")->Get();
    if (lootObject.IsEmpty())
        return false;

    Map* map = bot->GetMap();
    Creature* creature = map->GetCreature(lootObject.guid);
    GameObject* go = map->GetGameObject(lootObject.guid);

    WorldObject* wo = creature ? static_cast<WorldObject*>(creature) :
                                go ? static_cast<WorldObject*>(go) : nullptr;
    if (!wo)
        return false;

    if (bot->GetDistance(wo) > INTERACTION_DISTANCE - 2.0f)
        return false;

    if (creature && creature->IsAlive())
        return false;

    if (bot->IsMounted())
    {
        bot->Unmount(false);
        return true;
    }

    if (creature)
    {
        if (creature->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE))
        {
            bot->SendLoot(creature->GetObjectGuid(), LOOT_CORPSE);
            LOG_DEBUG("playerbots", "OpenLoot: %s looting creature entry %u", bot->GetName(), creature->GetEntry());
            return true;
        }
        return false;
    }

    if (go && go->GetGoState() == GO_STATE_READY)
    {
        go->UseDoorOrButton();
        return true;
    }

    return false;
}

bool StoreLootAction::Execute(Event event)
{
    ObjectGuid lootGuid = bot->GetLootGuid();
    if (!lootGuid)
    {
        LOG_DEBUG("playerbots", "%s [StoreLootAction] no lootGuid", bot->GetName());
        return false;
    }
    LOG_DEBUG("playerbots", "%s [StoreLootAction] lootGuid=%s", bot->GetName(), lootGuid.GetString().c_str());

    Map* map = bot->GetMap();
    Creature* creature = map->GetCreature(lootGuid);
    GameObject* go = map->GetGameObject(lootGuid);

    WorldObject* lootTarget = creature ? static_cast<WorldObject*>(creature) :
                                        go ? static_cast<WorldObject*>(go) : nullptr;
    if (!lootTarget)
        return false;

    Loot* loot = nullptr;
    if (creature)
        loot = &creature->loot;
    else if (go)
        loot = &go->loot;

    if (!loot || loot->empty())
    {
        LOG_DEBUG("playerbots", "%s [StoreLootAction] loot empty or null", bot->GetName());
        return false;
    }

    LOG_DEBUG("playerbots", "%s [StoreLootAction] loot has %u items, gold=%u", bot->GetName(), (uint32)loot->items.size(), loot->gold);

    LootStrategy* lootStrategy = GetAiObjectContext()->GetValue<LootStrategy*>("loot strategy")->Get();

    if (loot->gold > 0)
    {
        WorldPacket* pkt = new WorldPacket(CMSG_LOOT_MONEY, 0);
        bot->GetSession()->QueuePacket(pkt);
    }

    for (LootItemList::iterator iter = loot->items.begin(); iter != loot->items.end(); ++iter)
    {
        if (iter->is_looted)
            continue;

        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(iter->itemid);
        if (!proto)
        {
            LOG_DEBUG("playerbots", "%s [StoreLootAction] no proto for item %u", bot->GetName(), iter->itemid);
            continue;
        }

        if (lootStrategy && !lootStrategy->CanLoot(proto))
        {
            LOG_DEBUG("playerbots", "%s [StoreLootAction] skipped item %u '%s' (quality=%u)", bot->GetName(), iter->itemid, proto->Name1, proto->Quality);
            continue;
        }

        uint8 bagSpace = GetAiObjectContext()->GetValue<uint8>("bag space")->Get();
        if (bagSpace > 85)
        {
            if (proto->MaxCount > 1)
            {
                bool hasFreeStack = false;
                for (uint8 bag = 0; bag < INVENTORY_SLOT_BAG_END; ++bag)
                {
                    for (uint8 slot = 0; slot < MAX_BAG_SIZE; ++slot)
                    {
                        Item* existing = bot->GetItemByPos(bag, slot);
                        if (existing && existing->GetEntry() == iter->itemid)
                        {
                            if (existing->GetCount() + iter->count < proto->MaxCount)
                                hasFreeStack = true;
                        }
                    }
                    if (hasFreeStack)
                        break;
                }
                if (!hasFreeStack)
                    continue;
            }
        }

        WorldPacket* pkt = new WorldPacket(CMSG_AUTOSTORE_LOOT_ITEM, 1);
        *pkt << uint8(iter - loot->items.begin());
        bot->GetSession()->QueuePacket(pkt);

        LOG_DEBUG("playerbots", "StoreLoot: %s storing item %u '%s' count=%u",
            bot->GetName(), iter->itemid, proto->Name1, iter->count);

        iter->is_looted = true;
    }

    WorldPacket* pkt = new WorldPacket(CMSG_LOOT_RELEASE, 8);
    *pkt << lootGuid;
    bot->GetSession()->QueuePacket(pkt);

    LootObjectStack* stack = GetAiObjectContext()->GetValue<LootObjectStack*>("available loot")->Get();
    if (stack)
        stack->Remove(lootGuid);

    GetAiObjectContext()->GetValue<LootObject>("loot target")->Set(LootObject());
    return true;
}

bool EquipUpgradesAction::Execute([[maybe_unused]] Event event)
{
    uint32 equippedCount = 0;

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* currentItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!currentItem)
            continue;

        ItemPrototype const* currentProto = currentItem->GetProto();
        float currentScore = currentProto->ItemLevel * (currentProto->Quality + 1);

        for (uint8 bag = 1; bag < INVENTORY_SLOT_BAG_END; ++bag)
        {
            for (uint8 bagSlot = 0; bagSlot < MAX_BAG_SIZE; ++bagSlot)
            {
                Item* bagItem = bot->GetItemByPos(bag, bagSlot);
                if (!bagItem)
                    continue;

                ItemPrototype const* bagProto = bagItem->GetProto();
                if (bagProto->InventoryType == INVTYPE_NON_EQUIP)
                    continue;

                if (bot->CanUseItem(bagProto) != EQUIP_ERR_OK)
                    continue;

                uint8 targetSlot = NULL_SLOT;
                switch (bagProto->InventoryType)
                {
                    case INVTYPE_HEAD: targetSlot = EQUIPMENT_SLOT_HEAD; break;
                    case INVTYPE_NECK: targetSlot = EQUIPMENT_SLOT_NECK; break;
                    case INVTYPE_CLOAK: targetSlot = EQUIPMENT_SLOT_BACK; break;
                    case INVTYPE_CHEST: targetSlot = EQUIPMENT_SLOT_CHEST; break;
                    case INVTYPE_WAIST: targetSlot = EQUIPMENT_SLOT_WAIST; break;
                    case INVTYPE_LEGS: targetSlot = EQUIPMENT_SLOT_LEGS; break;
                    case INVTYPE_FEET: targetSlot = EQUIPMENT_SLOT_FEET; break;
                    case INVTYPE_WRISTS: targetSlot = EQUIPMENT_SLOT_WRISTS; break;
                    case INVTYPE_HANDS: targetSlot = EQUIPMENT_SLOT_HANDS; break;
                    case INVTYPE_FINGER:
                    case INVTYPE_TRINKET: targetSlot = EQUIPMENT_SLOT_FINGER1; break;
                    case INVTYPE_WEAPONMAINHAND:
                    case INVTYPE_2HWEAPON: targetSlot = EQUIPMENT_SLOT_MAINHAND; break;
                    case INVTYPE_WEAPONOFFHAND: targetSlot = EQUIPMENT_SLOT_OFFHAND; break;
                    case INVTYPE_RANGED: targetSlot = EQUIPMENT_SLOT_RANGED; break;
                    default: continue;
                }

                if (targetSlot == NULL_SLOT)
                    continue;

                float bagScore = bagProto->ItemLevel * (bagProto->Quality + 1);

                if (bagScore > currentScore * 1.1f)
                {
                    uint16 dest = 0;
                    InventoryResult invResult = bot->CanEquipItem(targetSlot, dest, bagItem, false);
                    if (invResult == EQUIP_ERR_OK)
                    {
                        bot->EquipItem(dest, bagItem, true);
                        LOG_DEBUG("playerbots", "EquipUpgrades: %s equipped %u '%s' (score %.1f > %.1f)",
                            bot->GetName(), bagProto->ItemId, bagProto->Name1, bagScore, currentScore);
                        ++equippedCount;
                        break;
                    }
                }
            }
        }
    }

    if (equippedCount > 0)
        LOG_DEBUG("playerbots", "EquipUpgrades: %s equipped %u items", bot->GetName(), equippedCount);

    return equippedCount > 0;
}

bool AddAllLootAction::Execute([[maybe_unused]] Event event)
{
    LootObjectStack* stack = GetAiObjectContext()->GetValue<LootObjectStack*>("available loot")->Get();
    if (!stack)
        return false;

    Map* map = bot->GetMap();
    if (!map || map->IsDungeon())
        return false;

    float scanRange = 100.0f;

    struct LootableCorpseVisitor
    {
        Player* player;
        LootObjectStack* lootStack;
        float scanRange;
        uint32 scanned;
        uint32 alive;
        uint32 dead;
        uint32 notLootable;
        uint32 tooFar;
        uint32 added;

        LootableCorpseVisitor(Player* p, LootObjectStack* stk, float range)
            : player(p), lootStack(stk), scanRange(range), scanned(0), alive(0), dead(0), notLootable(0), tooFar(0), added(0) {}

        void Visit(CreatureMapType &m)
        {
            for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            {
                Creature* c = itr->getSource();
                if (!c) continue;
                ++scanned;
                if (c->IsAlive()) { ++alive; continue; }
                ++dead;
                if (!c->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE)) { ++notLootable; continue; }
                float dist = player->GetDistance2d(c);
                if (dist > scanRange) { ++tooFar; continue; }
                lootStack->Add(c->GetGUID());
                ++added;
            }
        }

        void Visit(PlayerMapType &m) {}
        void Visit(CorpseMapType &) {}
        void Visit(CameraMapType &) {}
        void Visit(GameObjectMapType &) {}
        void Visit(DynamicObjectMapType &) {}
    } visitor(bot, stack, scanRange);

    CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();

    TypeContainerVisitor<LootableCorpseVisitor, WorldTypeMapContainer> world_vis(visitor);
    cell.Visit(p, world_vis, *map, *bot, scanRange);

    if (visitor.scanned > 0)
        LOG_DEBUG("playerbots", "AddAllLoot: %s scanned=%u alive=%u dead=%u notLootable=%u tooFar=%u added=%u",
            bot->GetName(), visitor.scanned, visitor.alive, visitor.dead, visitor.notLootable, visitor.tooFar, visitor.added);

    return true;
}

bool MoveToLootAction::Execute([[maybe_unused]] Event event)
{
    LootObject lootObject = GetAiObjectContext()->GetValue<LootObject>("loot target")->Get();
    if (lootObject.IsEmpty())
        return false;

    Map* map = bot->GetMap();
    Creature* creature = map->GetCreature(lootObject.guid);
    GameObject* go = map->GetGameObject(lootObject.guid);

    WorldObject* wo = creature ? static_cast<WorldObject*>(creature) :
                                go ? static_cast<WorldObject*>(go) : nullptr;
    if (!wo)
        return false;

    float x = wo->GetPositionX();
    float y = wo->GetPositionY();
    float z = wo->GetPositionZ();

    return MoveTo(x, y, z);
}
