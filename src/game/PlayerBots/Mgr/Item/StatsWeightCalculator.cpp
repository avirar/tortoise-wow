#include "StatsWeightCalculator.h"
#include "../../Util/SpecDetect.h"
#include "ItemPrototype.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharedDefines.h"
#include "SpellEntry.h"
#include "SpellMgr.h"
#include "DBCStores.h"

// Talent tab constants (vanilla: 3 trees per class)
// Tab 0, 1, 2 — meaning varies by class
// Warrior: 0=Arms, 1=Fury, 2=Protection
// Mage: 0=Arcane, 1=Fire, 2=Frost
// Warlock: 0=Affliction, 1=Demonology, 2=Destruction
// Priest: 0=Holy, 1=Shadow, 2=Discipline
// Paladin: 0=Holy, 1=Protection, 2=Retribution
// Hunter: 0=Beast Mastery, 1=Marksmanship, 2=Survival
// Rogue: 0=Assassination, 1=Combat, 2=Subtlety
// Shaman: 0=Elemental, 1=Enhancement, 2=Restoration
// Druid: 0=Feral Combat, 1=Restoration, 2=Balance

// Shared implementation lives in Util/SpecDetect.h (P1-3) — thin
// delegates keep the existing call sites unchanged.
static std::map<uint8, uint32> GetPlayerSpecTabs(Player* player)
{
    return PlayerbotSpec::GetTalentPointsPerTab(player);
}

static uint8 GetPlayerSpecTab(Player* player)
{
    return PlayerbotSpec::DetectSpecTab(player);
}

// Role detection based on class + spec
bool StatsWeightCalculator::IsMelee(Player* player)
{
    uint8 cls = player->GetClass();
    if (cls == CLASS_WARRIOR || cls == CLASS_ROGUE)
        return true;
    if (cls == CLASS_HUNTER)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 2; // Survival
    }
    if (cls == CLASS_DRUID)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 0; // Feral
    }
    if (cls == CLASS_PALADIN)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 2; // Retribution
    }
    if (cls == CLASS_SHAMAN)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 1; // Enhancement
    }
    return false;
}

bool StatsWeightCalculator::IsRanged(Player* player)
{
    uint8 cls = player->GetClass();
    if (cls == CLASS_HUNTER)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab != 2; // Not Survival
    }
    return false;
}

bool StatsWeightCalculator::IsCaster(Player* player)
{
    uint8 cls = player->GetClass();
    if (cls == CLASS_MAGE || cls == CLASS_WARLOCK)
        return true;
    if (cls == CLASS_DRUID)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 2; // Balance
    }
    if (cls == CLASS_PRIEST)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 1; // Shadow
    }
    if (cls == CLASS_SHAMAN)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 0; // Elemental
    }
    return false;
}

bool StatsWeightCalculator::IsHealer(Player* player)
{
    uint8 cls = player->GetClass();
    if (cls == CLASS_PRIEST)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab != 1; // Not Shadow
    }
    if (cls == CLASS_DRUID)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 1; // Restoration
    }
    if (cls == CLASS_PALADIN)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 0; // Holy
    }
    if (cls == CLASS_SHAMAN)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 2; // Restoration
    }
    return false;
}

bool StatsWeightCalculator::IsTank(Player* player)
{
    uint8 cls = player->GetClass();
    if (cls == CLASS_WARRIOR)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 2; // Protection
    }
    if (cls == CLASS_PALADIN)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 1; // Protection
    }
    if (cls == CLASS_DRUID)
    {
        uint8 tab = GetPlayerSpecTab(player);
        return tab == 0 && player->GetShapeshiftForm() == FORM_BEAR;
    }
    return false;
}

StatsWeightCalculator::StatsWeightCalculator(Player* player)
    : player_(player), collector_(nullptr)
{
    cls_ = player->GetClass();
    lvl_ = player->GetLevel();

    // Determine collector type based on role + spec
    if (IsTank(player))
        type_ = COLLECTOR_TANK;
    else if (IsHealer(player))
        type_ = COLLECTOR_HEALER;
    else if (IsCaster(player))
        type_ = COLLECTOR_CASTER;
    else if (IsRanged(player))
        type_ = COLLECTOR_RANGED;
    else
        type_ = COLLECTOR_MELEE;

    collector_ = new StatsCollector(type_, cls_);

    currentHit_ = 0.0f;
    currentCrit_ = 0.0f;
}

void StatsWeightCalculator::Reset()
{
    if (collector_)
        collector_->Reset();
    weight_ = 0.0f;
    for (uint32 i = 0; i < STATS_TYPE_MAX; ++i)
        statsWeights_[i] = 0.0f;
}

float StatsWeightCalculator::CalculateItem(uint32 itemId, int32 randomPropertyId)
{
    ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
    if (!proto)
        return 0.0f;

    // vanilla: a non-zero suffix id means an already-rolled instance suffix
    // (ITEM_FIELD_RANDOM_PROPERTIES_ID) — score it exactly; otherwise the
    // chance-weighted class average is used inside ScoreItem
    return ScoreItem(proto, randomPropertyId);
}

float StatsWeightCalculator::CalculateItem(Item* item)
{
    ItemPrototype const* proto = item ? item->GetProto() : nullptr;
    if (!proto)
        return 0.0f;

    return ScoreItem(proto, item->GetItemRandomPropertyId());
}

float StatsWeightCalculator::ScoreItem(ItemPrototype const* proto, int32 instanceSuffixId)
{
    Reset();

    // Collect base stats from item
    collector_->CollectItemStats(proto);

    // Collect stats from item spells (ON_EQUIP, ON_HIT, ON_USE)
    collector_->CollectItemSpells(proto);

    // Collect random suffix stats: instance-accurate when the rolled
    // suffix is known, else chance-weighted class average
    if (instanceSuffixId > 0)
        collector_->CollectRandomSuffixInstance(instanceSuffixId);
    else
        collector_->CollectRandomSuffix(proto);

    // Generate weights based on class/spec
    GenerateWeights(player_);

    // Calculate weighted score
    for (uint32 i = 0; i < STATS_TYPE_MAX; ++i)
        weight_ += statsWeights_[i] * collector_->stats[i];

    // Score floor: any equippable item must score > 0 (ItemUsageValue
    // treats score <= 0 as "never equip"), so low-level statless gray gear
    // still gets equipped. Deliberately SMALL and ilvl-weak: the stat/
    // armor/weapon-DPS weights must dominate. The old ilvl×10 REPLACE
    // fallback drowned real stat scores at higher levels (a +9 INT hat
    // scored 18 vs 490 for a statless hat of the same ilvl) and degenerated
    // the whole calculator back to crude ilvl ranking.
    float floor = 1.0f + float(proto->Quality) * 0.5f + float(proto->ItemLevel) * 0.05f;
    if (weight_ < floor)
        weight_ = floor;

    // Apply quality multiplier
    switch (proto->Quality)
    {
        case ITEM_QUALITY_UNCOMMON: weight_ *= 1.02f; break;
        case ITEM_QUALITY_RARE:     weight_ *= 1.05f; break;
        case ITEM_QUALITY_EPIC:     weight_ *= 1.10f; break;
        default: break;
    }

    // Apply armor type penalty if wrong type for class
    if (proto->Class == ITEM_CLASS_ARMOR)
    {
        bool canWearPlate = player_->HasSkill(SKILL_PLATE_MAIL);
        bool canWearMail = player_->HasSkill(SKILL_MAIL);
        bool canWearLeather = player_->HasSkill(SKILL_LEATHER);

        switch (proto->SubClass)
        {
            case ITEM_SUBCLASS_ARMOR_PLATE:
                if (!canWearPlate) weight_ *= 0.0f;
                break;
            case ITEM_SUBCLASS_ARMOR_MAIL:
                if (!canWearMail && !canWearPlate) weight_ *= 0.0f;
                else if (canWearPlate) weight_ *= 0.7f;
                break;
            case ITEM_SUBCLASS_ARMOR_LEATHER:
                if (!canWearLeather && !canWearMail && !canWearPlate) weight_ *= 0.0f;
                else if (canWearMail || canWearPlate) weight_ *= 0.7f;
                break;
            case ITEM_SUBCLASS_ARMOR_CLOTH:
                if (canWearMail || canWearPlate) weight_ *= 0.5f;
                break;
            default:
                break;
        }
    }

    return weight_;
}

void StatsWeightCalculator::GenerateWeights(Player* player)
{
    uint8 tab = GetPlayerSpecTab(player);
    
    // Base weights for all classes
    statsWeights_[STATS_TYPE_STAMINA] = 0.15f;
    statsWeights_[STATS_TYPE_ARMOR] = 0.001f;
    statsWeights_[STATS_TYPE_BONUS] = 1.0f;
    statsWeights_[STATS_TYPE_MELEE_DPS] = 0.01f;
    statsWeights_[STATS_TYPE_RANGED_DPS] = 0.01f;
    statsWeights_[STATS_TYPE_HEALTH_REGENERATION] = 0.05f;

    // Vanilla hit caps
    float hitCap = (type_ == COLLECTOR_CASTER || type_ == COLLECTOR_HEALER) ?
                   SPELL_HIT_CAP : MELEE_HIT_CAP;
    float hitWeight = 1.5f;

    // === WARRIOR ===
    if (cls_ == CLASS_WARRIOR)
    {
        if (tab == 2) // Protection
        {
            statsWeights_[STATS_TYPE_STRENGTH] = 1.5f;
            statsWeights_[STATS_TYPE_STAMINA] = 3.0f;
            statsWeights_[STATS_TYPE_ARMOR] = 0.15f;
            statsWeights_[STATS_TYPE_BLOCK_VALUE] = 2.0f;
            statsWeights_[STATS_TYPE_DODGE] = 1.5f;
            statsWeights_[STATS_TYPE_DEFENSE] = 2.0f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 1.0f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 0.5f;
        }
        else if (tab == 0) // Arms
        {
            statsWeights_[STATS_TYPE_STRENGTH] = 2.5f;
            statsWeights_[STATS_TYPE_AGILITY] = 0.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 2.0f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 7.0f;
        }
        else // Fury (tab == 1)
        {
            statsWeights_[STATS_TYPE_STRENGTH] = 2.5f;
            statsWeights_[STATS_TYPE_AGILITY] = 0.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.8f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 8.0f;
        }
    }
    // === PALADIN ===
    else if (cls_ == CLASS_PALADIN)
    {
        if (tab == 1) // Protection
        {
            statsWeights_[STATS_TYPE_STRENGTH] = 1.5f;
            statsWeights_[STATS_TYPE_STAMINA] = 2.5f;
            statsWeights_[STATS_TYPE_INTELLECT] = 0.5f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.3f;
            statsWeights_[STATS_TYPE_ARMOR] = 0.15f;
            statsWeights_[STATS_TYPE_BLOCK_VALUE] = 2.0f;
            statsWeights_[STATS_TYPE_DODGE] = 1.5f;
            statsWeights_[STATS_TYPE_DEFENSE] = 2.0f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 1.0f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 0.5f;
        }
        else if (tab == 0) // Holy
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 1.0f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_HEAL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.8f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_STAMINA] = 0.3f;
        }
        else // Retribution (tab == 2)
        {
            statsWeights_[STATS_TYPE_STRENGTH] = 2.0f;
            statsWeights_[STATS_TYPE_INTELLECT] = 0.8f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.3f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.5f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 0.8f;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 5.0f;
        }
    }
    // === ROGUE ===
    else if (cls_ == CLASS_ROGUE)
    {
        if (tab == 0) // Assassination
        {
            statsWeights_[STATS_TYPE_AGILITY] = 2.5f;
            statsWeights_[STATS_TYPE_STRENGTH] = 0.8f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 2.0f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 6.0f;
        }
        else if (tab == 1) // Combat
        {
            statsWeights_[STATS_TYPE_AGILITY] = 2.5f;
            statsWeights_[STATS_TYPE_STRENGTH] = 0.8f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.8f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 7.0f;
        }
        else // Subtlety (tab == 2)
        {
            statsWeights_[STATS_TYPE_AGILITY] = 2.5f;
            statsWeights_[STATS_TYPE_STRENGTH] = 0.8f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 2.0f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 5.0f;
        }
    }
    // === HUNTER ===
    else if (cls_ == CLASS_HUNTER)
    {
        if (tab == 2) // Survival (melee)
        {
            statsWeights_[STATS_TYPE_AGILITY] = 2.5f;
            statsWeights_[STATS_TYPE_STRENGTH] = 0.8f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.5f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 5.0f;
            statsWeights_[STATS_TYPE_RANGED_DPS] = 3.0f;
        }
        else if (tab == 1) // Marksmanship
        {
            statsWeights_[STATS_TYPE_AGILITY] = 2.5f;
            statsWeights_[STATS_TYPE_STRENGTH] = 0.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.5f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_RANGED_ATTACK_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_RANGED_DPS] = 10.0f;
        }
        else // Beast Mastery (tab == 0)
        {
            statsWeights_[STATS_TYPE_AGILITY] = 2.5f;
            statsWeights_[STATS_TYPE_STRENGTH] = 0.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.2f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_RANGED_DPS] = 8.0f;
        }
    }
    // === MAGE ===
    else if (cls_ == CLASS_MAGE)
    {
        if (tab == 2) // Frost
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.0f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.3f;
        }
        else if (tab == 1) // Fire
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.5f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.3f;
        }
        else // Arcane (tab == 0)
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.3f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 0.8f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.5f;
        }
    }
    // === WARLOCK ===
    else if (cls_ == CLASS_WARLOCK)
    {
        if (tab == 0) // Affliction
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.8f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 0.5f;
            statsWeights_[STATS_TYPE_STAMINA] = 0.5f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.3f;
        }
        else if (tab == 1) // Demonology
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 0.8f;
            statsWeights_[STATS_TYPE_STAMINA] = 0.3f;
        }
        else // Destruction (tab == 2)
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.8f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.2f;
            statsWeights_[STATS_TYPE_STAMINA] = 0.5f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.3f;
        }
    }
    // === PRIEST ===
    else if (cls_ == CLASS_PRIEST)
    {
        if (tab == 1) // Shadow
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.0f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.3f;
        }
        else if (tab == 0) // Holy
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 1.0f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_HEAL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.8f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_STAMINA] = 0.3f;
        }
        else // Discipline (tab == 2)
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.8f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_HEAL_POWER] = 1.2f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.6f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_STAMINA] = 0.3f;
        }
    }
    // === SHAMAN ===
    else if (cls_ == CLASS_SHAMAN)
    {
        if (tab == 2) // Restoration
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 1.0f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_HEAL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.8f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_STAMINA] = 0.3f;
        }
        else if (tab == 1) // Enhancement (melee)
        {
            statsWeights_[STATS_TYPE_STRENGTH] = 1.5f;
            statsWeights_[STATS_TYPE_AGILITY] = 1.0f;
            statsWeights_[STATS_TYPE_INTELLECT] = 0.8f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.3f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.5f;
            statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 0.8f;
            statsWeights_[STATS_TYPE_MELEE_DPS] = 5.0f;
        }
        else // Elemental (tab == 0)
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.0f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.3f;
        }
    }
    // === DRUID ===
    else if (cls_ == CLASS_DRUID)
    {
        if (tab == 0) // Feral
        {
            if (player && player->GetShapeshiftForm() == FORM_BEAR)
            {
                // Bear tank
                statsWeights_[STATS_TYPE_STRENGTH] = 2.0f;
                statsWeights_[STATS_TYPE_AGILITY] = 1.5f;
                statsWeights_[STATS_TYPE_STAMINA] = 3.0f;
                statsWeights_[STATS_TYPE_ARMOR] = 0.15f;
                statsWeights_[STATS_TYPE_DODGE] = 1.0f;
                statsWeights_[STATS_TYPE_HIT] = hitWeight;
                statsWeights_[STATS_TYPE_CRIT] = 1.0f;
                statsWeights_[STATS_TYPE_MELEE_DPS] = 2.0f;
            }
            else
            {
                // Cat DPS
                statsWeights_[STATS_TYPE_STRENGTH] = 2.0f;
                statsWeights_[STATS_TYPE_AGILITY] = 2.0f;
                statsWeights_[STATS_TYPE_STAMINA] = 0.5f;
                statsWeights_[STATS_TYPE_HIT] = hitWeight;
                statsWeights_[STATS_TYPE_CRIT] = 1.5f;
                statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
                statsWeights_[STATS_TYPE_MELEE_DPS] = 6.0f;
            }
        }
        else if (tab == 1) // Restoration
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 1.0f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.0f;
            statsWeights_[STATS_TYPE_HEAL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.8f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_STAMINA] = 0.3f;
        }
        else // Balance (tab == 2)
        {
            statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
            statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
            statsWeights_[STATS_TYPE_SPELL_POWER] = 1.5f;
            statsWeights_[STATS_TYPE_HIT] = hitWeight;
            statsWeights_[STATS_TYPE_CRIT] = 1.2f;
            statsWeights_[STATS_TYPE_MANA_REGENERATION] = 0.3f;
        }
    }
    // Fallback
    else
    {
        statsWeights_[STATS_TYPE_STRENGTH] = 1.0f;
        statsWeights_[STATS_TYPE_AGILITY] = 1.0f;
        statsWeights_[STATS_TYPE_INTELLECT] = 1.0f;
        statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
        statsWeights_[STATS_TYPE_HIT] = hitWeight;
        statsWeights_[STATS_TYPE_CRIT] = 1.0f;
    }
}
