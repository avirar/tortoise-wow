#include "LootObjectStack.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "Object.h"
#include "SharedDefines.h"

#define MAX_LOOT_OBJECT_COUNT 200

bool LootObjectStack::Add(ObjectGuid guid)
{
    if (!guid)
        return false;

    if (availableLoot_.size() >= MAX_LOOT_OBJECT_COUNT)
        availableLoot_.Shrink(time(nullptr) - 30);

    if (availableLoot_.size() >= MAX_LOOT_OBJECT_COUNT)
        availableLoot_.clear();

    LootTarget target(guid);
    return availableLoot_.insert(target).second;
}

void LootObjectStack::Remove(ObjectGuid guid)
{
    LootTargetList::iterator i = availableLoot_.find(guid);
    if (i != availableLoot_.end())
        availableLoot_.erase(i);
}

void LootObjectStack::Clear()
{
    availableLoot_.clear();
}

bool LootObjectStack::CanLoot(float maxDistance)
{
    LootObject nearest = GetNearest(maxDistance);
    return !nearest.IsEmpty();
}

LootObject LootObjectStack::GetLoot(float maxDistance)
{
    LootObject nearest = GetNearest(maxDistance);
    return nearest.IsEmpty() ? LootObject() : nearest;
}

LootObject LootObjectStack::GetNearest(float maxDistance)
{
    availableLoot_.Shrink(time(nullptr) - 30);

    LootObject nearest;
    float nearestDistance = std::numeric_limits<float>::max();

    Map* map = bot_->GetMap();
    if (!map)
        return LootObject();

    LootTargetList safeCopy(availableLoot_);
    for (LootTargetList::iterator i = safeCopy.begin(); i != safeCopy.end(); ++i)
    {
        ObjectGuid guid = i->guid;

        Creature* creature = map->GetCreature(guid);
        GameObject* go = map->GetGameObject(guid);

        WorldObject* worldObj = creature ? static_cast<WorldObject*>(creature) :
                                          go ? static_cast<WorldObject*>(go) : nullptr;
        if (!worldObj)
            continue;

        float distance = bot_->GetDistance(worldObj);

        if (distance >= nearestDistance || (maxDistance && distance > maxDistance))
            continue;

        if (creature && creature->IsAlive())
            continue;

        if (!creature && go)
        {
            if (go->GetGoState() != GO_STATE_READY)
                continue;
        }

        nearestDistance = distance;
        nearest = LootObject(guid);
    }

    return nearest;
}
