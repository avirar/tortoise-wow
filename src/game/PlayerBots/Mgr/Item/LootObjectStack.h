#ifndef _PLAYERBOT_LOOTOBJECTSTACK_H
#define _PLAYERBOT_LOOTOBJECTSTACK_H

#include "ObjectGuid.h"
#include <set>
#include <ctime>

class Player;
class WorldObject;

struct ItemPrototype;

class LootTarget
{
public:
    LootTarget() : guid(), asOfTime(0) {}
    LootTarget(ObjectGuid g) : guid(g), asOfTime(time(nullptr)) {}
    LootTarget(LootTarget const& other) : guid(other.guid), asOfTime(other.asOfTime) {}

    LootTarget& operator=(LootTarget const& other)
    {
        if (this != &other)
        {
            guid = other.guid;
            asOfTime = other.asOfTime;
        }
        return *this;
    }

    bool operator<(LootTarget const& other) const { return guid < other.guid; }

    ObjectGuid guid;
    time_t asOfTime;
};

class LootTargetList : public std::set<LootTarget>
{
public:
    void Shrink(time_t fromTime)
    {
        for (iterator i = begin(); i != end();)
        {
            if (i->asOfTime <= fromTime)
                erase(i++);
            else
                ++i;
        }
    }
};

class LootObject
{
public:
    LootObject() : guid() {}
    LootObject(ObjectGuid g) : guid(g) {}
    LootObject(LootObject const& other) : guid(other.guid) {}
    LootObject& operator=(LootObject const& other) = default;

    bool IsEmpty() const { return !guid; }
    void Clear() { guid.Clear(); }

    ObjectGuid guid;
};

class LootObjectStack
{
public:
    LootObjectStack(Player* bot) : bot_(bot) {}
    ~LootObjectStack() {}

    bool Add(ObjectGuid guid);
    void Remove(ObjectGuid guid);
    void Clear();
    bool CanLoot(float maxDistance);
    LootObject GetLoot(float maxDistance = 0);

private:
    LootObject GetNearest(float maxDistance);

    Player* bot_;
    LootTargetList availableLoot_;
};

#endif
