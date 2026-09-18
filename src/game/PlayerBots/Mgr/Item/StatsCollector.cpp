#include "StatsCollector.h"
#include "RandomSuffixCache.h"
#include "SharedDefines.h"
#include "SpellEntry.h"
#include "SpellMgr.h"
#include "SpellAuraDefines.h"
#include "DBCStores.h"
#include "Database/DBCEnums.h"

StatsCollector::StatsCollector(CollectorType type, uint8 cls) : type_(type), cls_(cls)
{
    Reset();
}

void StatsCollector::Reset()
{
    for (uint32 i = 0; i < STATS_TYPE_MAX; ++i)
        stats[i] = 0.0f;
}

bool StatsCollector::IsMeleeCollector() const
{
    return type_ == COLLECTOR_MELEE || type_ == COLLECTOR_TANK;
}

bool StatsCollector::IsRangedCollector() const
{
    return type_ == COLLECTOR_RANGED;
}

bool StatsCollector::IsCasterCollector() const
{
    return type_ == COLLECTOR_CASTER || type_ == COLLECTOR_HEALER;
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

    // Basic item stats (ItemStat array — only 7 stat types exist in this
    // data; the core's _ApplyItemBonuses ignores types >= 8, so unknown
    // types get NO credit: the engine would not apply them anyway)
    for (int i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
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
        case ITEM_MOD_MANA:
            // +X5 mana per 5 sec
            stats[STATS_TYPE_MANA_REGENERATION] += val;
            break;
        case ITEM_MOD_HEALTH:
            // +X5 health per 5 sec → stamina equivalent (15 HP = 1 STA)
            stats[STATS_TYPE_STAMINA] += val / 15.0f;
            break;
        default:
            // No credit: core ignores these stat types
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

        float multiplier = 0.0f;

        switch (proto->Spells[i].SpellTrigger)
        {
            case ITEM_SPELLTRIGGER_ON_EQUIP:
                // Always active → full weight
                multiplier = 1.0f;
                break;
            case ITEM_SPELLTRIGGER_ON_USE:
            case ITEM_SPELLTRIGGER_ON_NO_DELAY_USE:
                // On-use trinkets → reduced weight (uptime estimate)
                {
                    int32 cooldown = proto->Spells[i].SpellCooldown;
                    multiplier = (cooldown > 0) ? 0.15f : 0.25f;
                }
                break;
            case ITEM_SPELLTRIGGER_CHANCE_ON_HIT:
                // Procs: PPM-rate weighted when available, charges-based
                // estimate otherwise
                {
                    float ppm = proto->Spells[i].SpellPPMRate;
                    multiplier = (ppm > 0) ? std::min(ppm / 10.0f, 0.5f) : 0.2f;
                }
                break;
            default:
                // SOULSTONE etc. — no passive credit
                break;
        }

        if (multiplier > 0.0f)
            CollectSpellStats(spellId, multiplier);
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

        // Engine value (SpellEntry::CalculateSimpleValue):
        // base + die — item auras universally use die 0 or 1 (99% is 1),
        // so this matches the exact aura value the engine applies
        int32 avgValue = basePoints + dieSides;
        if (avgValue <= 0)
            continue;   // debuff/negative — no credit

        switch (effect)
        {
            case SPELL_EFFECT_APPLY_AURA:
            {
                // Core encoding (SpellAuras.cpp:337): the aura type is
                // EffectApplyAuraName[i]; EffectMiscValue[i] is the aura's
                // sub-parameter (for SPELL_AURA_MOD_STAT: the stat index)
                int32 auraType = (int32)spellInfo->EffectApplyAuraName[i];
                CollectAuraStats(auraType, avgValue, miscValue, multiplier);
                break;
            }
            case SPELL_EFFECT_HEAL:
                // One-shot healing (potions/bandages) — consumable value is
                // handled by the consumable layer, not gear scoring
                break;
            default:
                // No credit for unrecognized effects (no BONUS inflation)
                break;
        }
    }
}

void StatsCollector::CollectAuraStats(int32 auraType, int32 value, int32 miscValue, float multiplier)
{
    // Vanilla semantics: aura values are raw points — hit/crit are flat
    // percentage points (NOT ratings; no division needed), mirroring the
    // core's aura application (SpellAuras.cpp).

    switch (auraType)
    {
        // --- Hit chance (flat %), routed by collector type ---
        // Engine (HandleModHitChance): 54/184 apply to BOTH melee and
        // ranged hit chance, so both melee and ranged collectors get full
        // credit; 185 is ranged-only.
        case SPELL_AURA_MOD_HIT_CHANCE:                        // 54
        case SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE:         // 184
            stats[STATS_TYPE_HIT] += float(value) * multiplier * (IsMeleeCollector() || IsRangedCollector() ? 1.0f : 0.2f);
            break;
        case SPELL_AURA_MOD_SPELL_HIT_CHANCE:                  // 55
            stats[STATS_TYPE_HIT] += float(value) * multiplier * (IsCasterCollector() ? 1.0f : 0.2f);
            break;
        case SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE:        // 185
            stats[STATS_TYPE_HIT] += float(value) * multiplier * (IsRangedCollector() ? 1.0f : 0.2f);
            break;

        // --- Crit chance (flat %), routed by collector type ---
        // Engine (HandleAuraModCritPercent): generic 52 applies to BOTH
        // melee and ranged crit (weapon-class auras apply per weapon)
        case SPELL_AURA_MOD_CRIT_PERCENT:                      // 52
        case SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE:        // 187
            stats[STATS_TYPE_CRIT] += float(value) * multiplier * (IsMeleeCollector() || IsRangedCollector() ? 1.0f : 0.2f);
            break;
        case SPELL_AURA_MOD_SPELL_CRIT_CHANCE:                 // 57
        case SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE:        // 179
        case SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL:          // 71 (misc = school)
            stats[STATS_TYPE_CRIT] += float(value) * multiplier * (IsCasterCollector() ? 1.0f : 0.2f);
            break;
        case SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE:       // 188
            stats[STATS_TYPE_CRIT] += float(value) * multiplier * (IsRangedCollector() ? 1.0f : 0.2f);
            break;

        // --- Base stats: SPELL_AURA_MOD_STAT, miscValue = stat index ---
        // (core Aura::HandleAuraModStat: misc < 0 = all stats, 0..4 = index)
        case SPELL_AURA_MOD_STAT:
            if (miscValue < 0)
            {
                stats[STATS_TYPE_STRENGTH]    += float(value) * multiplier;
                stats[STATS_TYPE_AGILITY]     += float(value) * multiplier;
                stats[STATS_TYPE_INTELLECT]   += float(value) * multiplier;
                stats[STATS_TYPE_SPIRIT]      += float(value) * multiplier;
                stats[STATS_TYPE_STAMINA]     += float(value) * multiplier;
            }
            else if (miscValue >= 0 && miscValue <= 4)
            {
                switch (miscValue)
                {
                    case 0: stats[STATS_TYPE_STRENGTH]  += float(value) * multiplier; break;
                    case 1: stats[STATS_TYPE_AGILITY]   += float(value) * multiplier; break;
                    case 2: stats[STATS_TYPE_INTELLECT] += float(value) * multiplier; break;
                    case 3: stats[STATS_TYPE_SPIRIT]    += float(value) * multiplier; break;
                    case 4: stats[STATS_TYPE_STAMINA]   += float(value) * multiplier; break;
                    default: break;
                }
            }
            break;

        // --- Attack power ---
        case SPELL_AURA_MOD_ATTACK_POWER:
            stats[STATS_TYPE_ATTACK_POWER] += float(value) * multiplier;
            break;
        case SPELL_AURA_MOD_RANGED_ATTACK_POWER:
            stats[STATS_TYPE_RANGED_ATTACK_POWER] += float(value) * multiplier;
            break;

        // --- Spell power (vanilla: +X to [school] damage spells) ---
        case SPELL_AURA_MOD_DAMAGE_DONE:
            if (IsCasterCollector())
                stats[STATS_TYPE_SPELL_POWER] += float(value) * multiplier;
            break;

        // --- Healing power ---
        case SPELL_AURA_MOD_HEALING_DONE:
        case SPELL_AURA_MOD_HEALING:
            stats[STATS_TYPE_HEAL_POWER] += float(value) * multiplier;
            break;

        // --- Health → stamina equivalent (15 HP = 1 STA) ---
        case SPELL_AURA_MOD_INCREASE_HEALTH:
            stats[STATS_TYPE_STAMINA] += float(value) / 15.0f * multiplier;
            break;

        // --- Block value ---
        case SPELL_AURA_MOD_SHIELD_BLOCKVALUE:
            stats[STATS_TYPE_BLOCK_VALUE] += float(value) * multiplier;
            break;

        // --- Mana regen (vanilla: always mana) ---
        case SPELL_AURA_MOD_POWER_REGEN:
            stats[STATS_TYPE_MANA_REGENERATION] += float(value) * multiplier;
            break;

        // --- Health regen ---
        case SPELL_AURA_MOD_HEALTH_REGEN_PERCENT:
            stats[STATS_TYPE_HEALTH_REGENERATION] += float(value) * multiplier * 0.01f;
            break;

        // --- Dodge / parry / block ---
        // ONLY the percent auras are live in this engine — the *_SKILL
        // variants (46/48/50) map to HandleUnused (SpellAuras.cpp), so
        // crediting them would over-score gear that gives nothing.
        case SPELL_AURA_MOD_DODGE_PERCENT:                     // 49
            stats[STATS_TYPE_DODGE] += float(value) * multiplier;
            break;
        case SPELL_AURA_MOD_PARRY_PERCENT:                     // 47
            stats[STATS_TYPE_PARRY] += float(value) * multiplier;
            break;
        case SPELL_AURA_MOD_BLOCK_PERCENT:                     // 51
            stats[STATS_TYPE_BLOCK_RATING] += float(value) * multiplier;
            break;

        // --- Haste (raw % attack speed; aura value IS the percent —
        // HandleModMeleeSpeedPct applies m_amount directly to attack time)
        case SPELL_AURA_MOD_MELEE_HASTE:                       // 138
        case SPELL_AURA_MOD_RANGED_HASTE:                      // 140
        case SPELL_AURA_MOD_RANGED_AMMO_HASTE:                 // 141
            stats[STATS_TYPE_HASTE] += float(value) * multiplier;
            break;

        // --- Defense skill: MOD_SKILL with miscValue = SKILL_DEFENSE ---
        case SPELL_AURA_MOD_SKILL:
            if (miscValue == SKILL_DEFENSE)
                stats[STATS_TYPE_DEFENSE] += float(value) * multiplier;
            break;

        // --- Resistances → armor proxy (simplified, as before) ---
        case SPELL_AURA_MOD_RESISTANCE:
        case SPELL_AURA_MOD_RESISTANCE_PCT:
        case SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE:
            stats[STATS_TYPE_ARMOR] += float(value) * 0.1f * multiplier;
            break;

        default:
            // No credit for unrecognized auras (no BONUS inflation)
            break;
    }
}

void StatsCollector::CollectEnchantStats(uint32 enchantId, float multiplier)
{
    if (!enchantId || multiplier <= 0.0f)
        return;

    SpellItemEnchantmentEntry const* ench = sSpellItemEnchantmentStore.LookupEntry(enchantId);
    if (!ench)
        return;

    // Vanilla enchant entry: type[s] = ITEM_ENCHANTMENT_TYPE_* (see core
    // Player::ApplyEnchantment), amount[s] = value, spellid[s] = stat
    // index (for TYPE_STAT), school (TYPE_RESISTANCE) or spell (others)
    for (int s = 0; s < 3; ++s)
    {
        uint32 enchType = ench->type[s];
        uint32 amount = ench->amount[s];
        uint32 spellId = ench->spellid[s];
        if (!enchType)
            continue;

        switch (enchType)
        {
            case ITEM_ENCHANTMENT_TYPE_STAT:
                // spellId holds the stat index (ITEM_MOD_* numbering,
                // identical to item stat types)
                if (amount)
                    CollectByItemStatType(spellId, (int32)amount);
                break;
            case ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL:
                // Passive aura spell (the common case: "+1 Strength" is an
                // EQUIP_SPELL granting SPELL_AURA_MOD_STAT)
                if (spellId)
                    CollectSpellStats(spellId, multiplier);
                break;
            case ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL:
                // Weapon-attack proc (rate lives in spell_proc_item_enchant,
                // not in this entry) — conservative fixed weight for v1
                if (spellId)
                    CollectSpellStats(spellId, multiplier * 0.25f);
                break;
            case ITEM_ENCHANTMENT_TYPE_TOTEM:
                // Shaman weapon proc (rockbiter family)
                if (spellId)
                    CollectSpellStats(spellId, multiplier * 0.25f);
                else if (amount)
                    stats[STATS_TYPE_MELEE_DPS] += float(amount) * multiplier * 0.25f;
                break;
            case ITEM_ENCHANTMENT_TYPE_DAMAGE:
                // Flat weapon damage bonus per swing → DPS approx at 2.5s
                if (amount)
                    stats[STATS_TYPE_MELEE_DPS] += float(amount) * multiplier / 2.5f;
                break;
            case ITEM_ENCHANTMENT_TYPE_RESISTANCE:
                // spellId = school — resistances get the armor proxy
                if (amount)
                    stats[STATS_TYPE_ARMOR] += float(amount) * 0.1f * multiplier;
                break;
            default:
                break;
        }
    }
}

void StatsCollector::CollectRandomSuffixInstance(int32 randomPropertyId)
{
    if (randomPropertyId <= 0)
        return;

    // Instance-accurate: the suffix was already rolled onto the item
    ItemRandomPropertiesEntry const* entry = sItemRandomPropertiesStore.LookupEntry(uint32(randomPropertyId));
    if (!entry)
        return;

    for (int i = 0; i < 3; ++i)
        if (entry->enchant_id[i])
            CollectEnchantStats(entry->enchant_id[i], 1.0f);
}

void StatsCollector::CollectRandomSuffix(ItemPrototype const* proto)
{
    if (!proto || !proto->RandomProperty)
        return;

    // Proto-level scoring: the core rolls ONE suffix per instance
    // (weighted by item_enchantment_template.chance), so the honest score
    // is the chance-weighted AVERAGE over the class.
    const std::vector<SuffixOption>* options = RandomSuffixCache::GetOptions(proto->RandomProperty);
    if (!options || options->empty())
        return;

    float totalChance = 0.0f;
    for (const SuffixOption& opt : *options)
        totalChance += opt.chance;
    if (totalChance <= 0.0f)
        return;

    for (const SuffixOption& opt : *options)
    {
        ItemRandomPropertiesEntry const* entry = sItemRandomPropertiesStore.LookupEntry(opt.suffixId);
        if (!entry)
            continue;

        float weight = opt.chance / totalChance;
        for (int i = 0; i < 3; ++i)
            if (entry->enchant_id[i])
                CollectEnchantStats(entry->enchant_id[i], weight);
    }
}

void StatsCollector::CollectSocketBonus(ItemPrototype const* /*proto*/)
{
    // No-op by design: this data has no socket columns (item_template and
    // ItemPrototype both lack them), so there is nothing to score.
}
