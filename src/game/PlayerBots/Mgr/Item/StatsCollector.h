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

    // Collect green-suffix stats for a proto-only item: chance-weighted
    // average over the item_enchantment_template roll class (instances
    // roll one suffix, so the expected value is the mean — see
    // RandomSuffixCache.h for the full vanilla suffix chain).
    void CollectRandomSuffix(ItemPrototype const* proto);

    // Collect green-suffix stats for a concrete instance: the rolled
    // suffix id (Item::GetItemRandomPropertyId), scored exactly.
    void CollectRandomSuffixInstance(int32 randomPropertyId);

    // Collect stats from one enchantment (suffix enchants and, later,
    // applied perm enchants share this path).
    void CollectEnchantStats(uint32 enchantId, float multiplier = 1.0f);

    // Socket bonus: NO-OP by design — this data has no socket columns
    // (item_template / ItemPrototype both lack them; vanilla socket DBC
    // fields were not ported). Kept for signature parity.
    void CollectSocketBonus(ItemPrototype const* proto);

    float stats[STATS_TYPE_MAX];

private:
    void CollectByItemStatType(uint32 itemStatType, int32 val);
    void CollectSpellStats(uint32 spellId, float multiplier);
    void CollectAuraStats(int32 auraType, int32 value, int32 miscValue, float multiplier);

    // Collector-type routing for hit/crit (primary family full weight,
    // cross families at 0.2 — e.g. a survival hunter still gets partial
    // credit for spell-hit gear rather than zero)
    bool IsMeleeCollector() const;
    bool IsRangedCollector() const;
    bool IsCasterCollector() const;

    CollectorType type_;
    uint8 cls_;
};

#endif
