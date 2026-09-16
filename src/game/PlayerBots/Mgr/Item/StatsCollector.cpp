#include "StatsCollector.h"
#include "SharedDefines.h"
#include "SpellEntry.h"
#include "SpellMgr.h"
#include "SpellAuraDefines.h"
#include "DBCStores.h"

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

    // Weapon DPS calculation
    if (proto->IsRangedWeapon())
    {
        if (proto->Delay > 0)
        {
            float val = (proto->Damage[0].DamageMin + proto->Damage[0].DamageMax) * 1000.0f / 2.0f / proto->Delay;
            stats[STATS_TYPE_RANGED_DPS] += val;
        }
    }
    else if (proto->IsWeapon())
    {
        if (proto->Delay > 0)
        {
            float val = (proto->Damage[0].DamageMin + proto->Damage[0].DamageMax) * 1000.0f / 2.0f / proto->Delay;
            stats[STATS_TYPE_MELEE_DPS] += val;
        }
    }

    // Armor and block value
    stats[STATS_TYPE_ARMOR] += proto->Armor;
    stats[STATS_TYPE_BLOCK_VALUE] += proto->Block;

    // Basic item stats (ItemStat array)
    for (int i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        uint32 statType = proto->ItemStat[i].ItemStatType;
        int32 val = proto->ItemStat[i].ItemStatValue;
        if (val == 0)
            continue;
        CollectByItemStatType(statType, val);
    }

    // Mana regeneration from ITEM_MOD_MANA (+X5 mana per 5 sec)
    // Already handled in CollectByItemStatType

    // Health regeneration from ITEM_MOD_HEALTH (+X5 health per 5 sec)
    // Already handled in CollectByItemStatType
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
        case ITEM_MOD_MANA:
            // +X5 mana per 5 sec → normalize to per-second
            stats[STATS_TYPE_MANA_REGENERATION] += val;
            break;
        case ITEM_MOD_HEALTH:
            // +X5 health per 5 sec → stamina equivalent
            stats[STATS_TYPE_STAMINA] += val / 15.0f;
            break;
        default:
            // Unknown stat type → bonus
            stats[STATS_TYPE_BONUS] += 1.0f;
            break;
    }
}

void StatsCollector::CollectItemSpells(ItemPrototype const* proto)
{
    if (!proto)
        return;

    for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        uint32 spellId = proto->Spells[i].SpellId;
        if (!spellId)
            continue;

        // Only process certain triggers
        switch (proto->Spells[i].SpellTrigger)
        {
            case ITEM_SPELLTRIGGER_ON_EQUIP:
                // Always active → full weight
                CollectSpellStats(spellId, 1.0f);
                break;
            case ITEM_SPELLTRIGGER_ON_USE:
                // On-use trinkets → reduced weight based on cooldown
                {
                    int32 cooldown = proto->Spells[i].SpellCooldown;
                    if (cooldown > 0)
                    {
                        // Uptime = duration / (duration + cooldown)
                        // For simplicity, use a fixed multiplier for on-use effects
                        float multiplier = 0.15f; // ~15% uptime estimate
                        CollectSpellStats(spellId, multiplier);
                    }
                    else
                    {
                        CollectSpellStats(spellId, 0.25f);
                    }
                }
                break;
            case ITEM_SPELLTRIGGER_CHANCE_ON_HIT:
                // Chance on hit → weight by PPM rate
                {
                    float ppm = proto->Spells[i].SpellPPMRate;
                    if (ppm > 0)
                    {
                        // PPM rate → approximate uptime
                        // 1.8 PPM ≈ 1.8% per minute ≈ very low
                        // 5.0 PPM ≈ moderate proc rate
                        float multiplier = std::min(ppm / 10.0f, 0.5f);
                        CollectSpellStats(spellId, multiplier);
                    }
                }
                break;
            default:
                break;
        }
    }
}

void StatsCollector::CollectSpellStats(uint32 spellId, float multiplier)
{
    SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellId);
    if (!spellInfo)
        return;

    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        uint32 effect = spellInfo->Effect[i];
        if (!effect)
            continue;

        int32 basePoints = spellInfo->EffectBasePoints[i];
        int32 dieSides = spellInfo->EffectDieSides[i];
        int32 miscValue = spellInfo->EffectMiscValue[i];

        // Average value = basePoints + (dieSides + 1) / 2
        int32 avgValue = basePoints + (dieSides > 0 ? (dieSides + 1) / 2 : 0);
        if (avgValue <= 0)
            continue;

        switch (effect)
        {
            case SPELL_EFFECT_APPLY_AURA:
                CollectAuraStats(miscValue, avgValue, multiplier);
                break;
            case SPELL_EFFECT_HEAL:
                // Healing over time → heal power equivalent
                stats[STATS_TYPE_HEAL_POWER] += avgValue * multiplier;
                break;
            default:
                // Unknown effect → small bonus
                stats[STATS_TYPE_BONUS] += 0.5f * multiplier;
                break;
        }
    }
}

void StatsCollector::CollectAuraStats(int32 auraType, int32 value, float multiplier)
{
    // auraType = EffectMiscValue from spell entry (SPELL_AURA_* constant)
    // value = EffectBasePoints + dieSides average (the numeric value)
    // In vanilla WoW's SpellEntry, for SPELL_EFFECT_APPLY_AURA:
    //   EffectMiscValue = aura type (SPELL_AURA_*)
    //   The aura's own sub-parameters (e.g., which stat for MOD_STAT)
    //   are NOT in the spell entry — they're in the aura application layer.
    // We handle what we can from the spell entry alone.

    switch (auraType)
    {
        case SPELL_AURA_MOD_STAT:
            // Vanilla SpellEntry doesn't expose which stat — treat as bonus
            // Common item spells: +X All Stats, +X Strength, etc.
            // Without the stat index, credit it as a small bonus
            stats[STATS_TYPE_BONUS] += value * multiplier * 0.5f;
            break;

        case SPELL_AURA_MOD_ATTACK_POWER:
            stats[STATS_TYPE_ATTACK_POWER] += value * multiplier;
            break;

        case SPELL_AURA_MOD_RANGED_ATTACK_POWER:
            stats[STATS_TYPE_RANGED_ATTACK_POWER] += value * multiplier;
            break;

        case SPELL_AURA_MOD_DAMAGE_DONE:
            // Spell damage bonus → spell power equivalent
            if (type_ & (COLLECTOR_CASTER | COLLECTOR_HEALER))
                stats[STATS_TYPE_SPELL_POWER] += value * multiplier;
            break;

        case SPELL_AURA_MOD_HEALING_DONE:
            stats[STATS_TYPE_HEAL_POWER] += value * multiplier;
            break;

        case SPELL_AURA_MOD_INCREASE_HEALTH:
            // +X health → stamina equivalent (15 HP = 1 stamina)
            stats[STATS_TYPE_STAMINA] += value / 15.0f * multiplier;
            break;

        case SPELL_AURA_MOD_RESISTANCE:
        case SPELL_AURA_MOD_RESISTANCE_PCT:
        case SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE:
            // Resistance → armor equivalent (simplified)
            stats[STATS_TYPE_ARMOR] += value * 0.1f * multiplier;
            break;

        case SPELL_AURA_MOD_SHIELD_BLOCKVALUE:
            stats[STATS_TYPE_BLOCK_VALUE] += value * multiplier;
            break;

        case SPELL_AURA_MOD_POWER_REGEN:
            // Mana regen — vanilla items with this aura always target mana
            stats[STATS_TYPE_MANA_REGENERATION] += value * multiplier;
            break;

        case SPELL_AURA_MOD_HEALTH_REGEN_PERCENT:
            // Health regen % → health regen equivalent
            stats[STATS_TYPE_HEALTH_REGENERATION] += value * multiplier * 0.01f;
            break;

        case SPELL_AURA_MOD_HIT_CHANCE:
            // Hit chance aura → hit percentage
            stats[STATS_TYPE_HIT] += value * multiplier * 0.02f;
            break;

        case SPELL_AURA_MOD_CRIT_PERCENT:
            // Crit chance aura → crit percentage
            stats[STATS_TYPE_CRIT] += value * multiplier * 0.02f;
            break;

        case SPELL_AURA_MOD_DODGE_SKILL:
        case SPELL_AURA_MOD_DODGE_PERCENT:
            stats[STATS_TYPE_DODGE] += value * multiplier;
            break;

        case SPELL_AURA_MOD_PARRY_SKILL:
        case SPELL_AURA_MOD_PARRY_PERCENT:
            stats[STATS_TYPE_PARRY] += value * multiplier;
            break;

        case SPELL_AURA_MOD_BLOCK_SKILL:
        case SPELL_AURA_MOD_BLOCK_PERCENT:
            stats[STATS_TYPE_BLOCK_RATING] += value * multiplier;
            break;

        default:
            // Unknown aura → small bonus
            stats[STATS_TYPE_BONUS] += 0.5f * multiplier;
            break;
    }
}

void StatsCollector::CollectRandomSuffix(ItemPrototype const* proto)
{
    if (!proto || !proto->RandomProperty)
        return;

    // In vanilla, RandomProperty points to ItemRandomSuffix.dbc
    // which has enchant IDs. We need to look up the enchant and process its stats.
    // For vanilla, most random suffixes are simple stat bonuses handled by
    // the ItemStat array already. This is a fallback for any additional effects.
    
    // Check if there are spells associated with the random property
    // (some suffix items have additional spell effects)
    uint32 randomPropId = proto->RandomProperty;
    
    // In vanilla, random suffix stats are already in the ItemStat array
    // This method is a placeholder for future expansion
}

void StatsCollector::CollectSocketBonus(ItemPrototype const* proto)
{
    if (!proto)
        return;

    // Count sockets for bonus evaluation
    // In vanilla, sockets are rare and meta gems are limited
    // Socket bonuses are typically small stat boosts
    
    // Note: vanilla ItemPrototype doesn't have a socket bonus field like WotLK
    // Socket bonuses are handled through item spells in vanilla
    // This is a placeholder for future expansion
}
