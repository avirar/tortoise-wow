#include "LootTriggers.h"
#include "PlayerBotAI.h"
#include "AiObjectContext.h"
#include "Value.h"
#include "Creature.h"
#include "GameObject.h"
#include "Mgr/Item/LootObjectStack.h"
#include "SharedDefines.h"
#include "Logging.h"
#include "Log.h"

bool LootAvailableTrigger::IsActive()
{
    LootObjectStack* stack = GetAiObjectContext()->GetValue<LootObjectStack*>("available loot")->Get();
    if (!stack)
        return false;
    return stack->CanLoot(INTERACTION_DISTANCE);
}

bool FarFromLootTrigger::IsActive()
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

    return bot->GetDistance2d(wo) > INTERACTION_DISTANCE - 2.0f;
}

bool CanLootTrigger::IsActive()
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

bool LootOpenTrigger::IsActive()
{
    // AC WorldPacketHandlerStrategy pattern: "loot response" fires when server
    // sends SMSG_LOOT_RESPONSE after opening a loot frame.
    // We approximate this by checking if the bot has an active loot GUID.
    ObjectGuid lootGuid = bot->GetLootGuid();
    if (!lootGuid.IsEmpty())
    {
        LOG_DEBUG("playerbots", "%s [LootOpenTrigger] lootGuid=%s", bot->GetName(), lootGuid.GetString().c_str());
        return true;
    }
    return false;
}
