#ifndef _PLAYERBOT_STATSCOLLECTOR_H
#define _PLAYERBOT_STATSCOLLECTOR_H

#include "ItemPrototype.h"
#include "Common.h"

// Stats types for item evaluation
// Vanilla WoW uses raw percentages for hit/crit/etc, not ratings
enum StatsType : uint8
{
    // Basic stats
    STATS_TYPE_AGILITY = 0,
    STATS_TYPE_STRENGTH,
    STATS_TYPE_INTELLECT,
    STATS_TYPE_SPIRIT,
    STATS_TYPE_STAMINA,
    // Combat ratings (vanilla: raw percentages from item spells)
    STATS_TYPE_HIT,           // hit chance %
    STATS_TYPE_CRIT,          // crit chance %
    STATS_TYPE_HASTE,         // haste % (rare in vanilla)
    // Tanking stats
    STATS_TYPE_ARMOR,
    STATS_TYPE_DEFENSE,
    STATS_TYPE_DODGE,
    STATS_TYPE_PARRY,
    STATS_TYPE_BLOCK_VALUE,
    STATS_TYPE_BLOCK_RATING,
    // Spell power
    STATS_TYPE_SPELL_POWER,
    STATS_TYPE_HEAL_POWER,
    // Physical damage
    STATS_TYPE_ATTACK_POWER,
    STATS_TYPE_RANGED_ATTACK_POWER,
    // Regeneration
    STATS_TYPE_MANA_REGENERATION,
    STATS_TYPE_HEALTH_REGENERATION,
    // Weapon DPS
    STATS_TYPE_MELEE_DPS,
    STATS_TYPE_RANGED_DPS,
    // Bonus for unrecognized stats
    STATS_TYPE_BONUS,
    STATS_TYPE_MAX
};

// Collector type determines which hit/crit ratings are relevant
enum CollectorType : uint8
{
    COLLECTOR_MELEE   = 1,
    COLLECTOR_RANGED  = 2,
    COLLECTOR_CASTER  = 4,
    COLLECTOR_HEALER  = 8,
    COLLECTOR_TANK    = 16
};

class StatsCollector
{
public:
    StatsCollector(CollectorType type, uint8 cls = 0);
    void Reset();
    
    // Collect stats from item prototype (base stats + armor + weapon DPS)
    void CollectItemStats(ItemPrototype const* proto);
    
    // Collect stats from item spells (ON_EQUIP, ON_HIT, ON_USE)
    void CollectItemSpells(ItemPrototype const* proto);
    
    // Collect stats from random suffix (+X Strength green text)
    void CollectRandomSuffix(ItemPrototype const* proto);
    
    // Collect stats from socket bonus
    void CollectSocketBonus(ItemPrototype const* proto);

    float stats[STATS_TYPE_MAX];

private:
    void CollectByItemStatType(uint32 itemStatType, int32 val);
    void CollectSpellStats(uint32 spellId, float multiplier = 1.0f);
    void CollectAuraStats(int32 auraType, int32 value, float multiplier);
    
    CollectorType type_;
    uint8 cls_;
};

#endif
