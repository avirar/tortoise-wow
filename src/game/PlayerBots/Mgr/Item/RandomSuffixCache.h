/*
 * The Grand Protocol v3/gpl-3.0 License
 * tortoise-wow PlayerBot engine port — green-item suffix options cache
 *
 * Vanilla suffix model (verified against core + live data 2026-09-18):
 *   item_template.random_property  = suffix CLASS id
 *   item_enchantment_template (SQL: entry/ench/chance) = roll table,
 *       `ench` = ItemRandomProperties DBC id (the suffix, e.g. "of Strength")
 *   ItemRandomPropertiesEntry.enchant_id[3] = SpellItemEnchantment ids
 *   SpellItemEnchantmentEntry (type/amount/spellid per slot) = the stats,
 *       mostly type=EQUIP_SPELL -> item-aura spell (e.g. +1 Strength)
 *
 * The core rolls one suffix per instance (Item::GenerateItemRandomPropertyId,
 * in-memory `RandomItemEnch`), so the honest score for a proto-level item is
 * the chance-weighted AVERAGE over the class — not AC's optimistic
 * "best suffix" (that data model is WotLK ItemRandomProperties + allocation).
 * For items in a bot's bag, the rolled suffix is already on the instance
 * (Item::GetItemRandomPropertyId) and is scored exactly.
 */
#ifndef PLAYERBOT_RANDOM_SUFFIX_CACHE_H
#define PLAYERBOT_RANDOM_SUFFIX_CACHE_H

#include <vector>
#include <unordered_map>
#include "Common.h"

// One roll option for a suffix class (mirrors core EnchStoreItem)
struct SuffixOption
{
    uint32 suffixId;    // ItemRandomProperties DBC id
    float chance;       // weighted chance from item_enchantment_template
};

namespace RandomSuffixCache
{
    // One-time load of `item_enchantment_template` (same filter as core
    // LoadRandomEnchantmentsTable: chance > 0 && chance <= 100).
    // Safe to call repeatedly (no-op after first load).
    void EnsureLoaded();

    // Suffix options for a random_property class id; nullptr if none/unknown.
    const std::vector<SuffixOption>* GetOptions(uint32 entry);

    // True while the cache has been loaded (even with 0 rows).
    bool IsLoaded();
}

#endif
