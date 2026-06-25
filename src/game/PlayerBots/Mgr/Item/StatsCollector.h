#ifndef _PLAYERBOT_STATSCOLLECTOR_H
#define _PLAYERBOT_STATSCOLLECTOR_H

#include "ItemPrototype.h"
#include "Common.h"

enum StatsType : uint8
{
    STATS_TYPE_STRENGTH = 0,
    STATS_TYPE_AGILITY,
    STATS_TYPE_STAMINA,
    STATS_TYPE_INTELLECT,
    STATS_TYPE_SPIRIT,
    STATS_TYPE_ARMOR,
    STATS_TYPE_HIT,
    STATS_TYPE_CRIT,
    STATS_TYPE_MELEE_DPS,
    STATS_TYPE_RANGED_DPS,
    STATS_TYPE_ATTACK_POWER,
    STATS_TYPE_DEFENSE,
    STATS_TYPE_DODGE,
    STATS_TYPE_BONUS,
    STATS_TYPE_MAX
};

enum CollectorType : uint8
{
    COLLECTOR_MELEE = 1,
    COLLECTOR_RANGED = 2,
    COLLECTOR_CASTER = 4,
    COLLECTOR_TANK = 8
};

class StatsCollector
{
public:
    StatsCollector(CollectorType type, uint8 cls = 0);
    void Reset();
    void CollectItemStats(ItemPrototype const* proto);

    float stats[STATS_TYPE_MAX];

private:
    void CollectByItemStatType(uint32 itemStatType, int32 val);

    CollectorType type_;
    uint8 cls_;
};

#endif
