#include "StatsCollector.h"
#include "SharedDefines.h"

StatsCollector::StatsCollector(CollectorType type, uint8 cls) : type_(type), cls_(cls)
{
    Reset();
}

void StatsCollector::Reset()
{
    for (uint32 i = 0; i < STATS_TYPE_MAX; ++i)
        stats[i] = 0.0f;
}

void StatsCollector::CollectItemStats(ItemPrototype const* proto)
{
    if (!proto)
        return;

    if (proto->IsRangedWeapon())
    {
        float val = (proto->Damage[0].DamageMin + proto->Damage[0].DamageMax) * 1000.0f / 2.0f / proto->Delay;
        stats[STATS_TYPE_RANGED_DPS] += val;
    }
    else if (proto->IsWeapon())
    {
        float val = (proto->Damage[0].DamageMin + proto->Damage[0].DamageMax) * 1000.0f / 2.0f / proto->Delay;
        stats[STATS_TYPE_MELEE_DPS] += val;
    }

    stats[STATS_TYPE_ARMOR] += proto->Armor;

    for (int i = 0; i < 10; ++i)
    {
        uint32 statType = proto->ItemStat[i].ItemStatType;
        int32 val = proto->ItemStat[i].ItemStatValue;
        if (val == 0)
            continue;
        CollectByItemStatType(statType, val);
    }
}

void StatsCollector::CollectByItemStatType(uint32 itemStatType, int32 val)
{
    switch (itemStatType)
    {
        case ITEM_MOD_AGILITY:
            stats[STATS_TYPE_AGILITY] += val;
            break;
        case ITEM_MOD_STRENGTH:
            stats[STATS_TYPE_STRENGTH] += val;
            break;
        case ITEM_MOD_INTELLECT:
            stats[STATS_TYPE_INTELLECT] += val;
            break;
        case ITEM_MOD_SPIRIT:
            stats[STATS_TYPE_SPIRIT] += val;
            break;
        case ITEM_MOD_STAMINA:
            stats[STATS_TYPE_STAMINA] += val;
            break;
        default:
            break;
    }
}
