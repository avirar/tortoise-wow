#include "StatsWeightCalculator.h"
#include "ItemPrototype.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharedDefines.h"

StatsWeightCalculator::StatsWeightCalculator(Player* player)
    : player_(player), cls_(player->GetClass()), lvl_(player->GetLevel()), collector_(nullptr)
{
    if (cls_ == CLASS_WARRIOR || cls_ == CLASS_DRUID)
        type_ = COLLECTOR_MELEE;
    else if (cls_ == CLASS_HUNTER)
        type_ = COLLECTOR_RANGED;
    else if (cls_ == CLASS_MAGE || cls_ == CLASS_WARLOCK || cls_ == CLASS_PRIEST)
        type_ = COLLECTOR_CASTER;
    else
        type_ = COLLECTOR_MELEE;

    collector_ = new StatsCollector(type_, cls_);
}

void StatsWeightCalculator::Reset()
{
    if (collector_)
        collector_->Reset();
    weight_ = 0.0f;
    for (uint32 i = 0; i < STATS_TYPE_MAX; ++i)
        statsWeights_[i] = 0.0f;
}

float StatsWeightCalculator::CalculateItem(uint32 itemId, int32 /*randomPropertyId*/)
{
    ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
    if (!proto)
        return 0.0f;

    Reset();
    collector_->CollectItemStats(proto);
    GenerateWeights(player_);

    for (uint32 i = 0; i < STATS_TYPE_MAX; ++i)
        weight_ += statsWeights_[i] * collector_->stats[i];

    if (proto->Quality >= ITEM_QUALITY_UNCOMMON)
        weight_ *= 1.05f;
    if (proto->Quality >= ITEM_QUALITY_RARE)
        weight_ *= 1.1f;
    if (proto->Quality >= ITEM_QUALITY_EPIC)
        weight_ *= 1.15f;

    return weight_;
}

void StatsWeightCalculator::GenerateWeights(Player* /*player*/)
{
    statsWeights_[STATS_TYPE_STAMINA] = 0.1f;
    statsWeights_[STATS_TYPE_ARMOR] = 0.001f;
    statsWeights_[STATS_TYPE_BONUS] = 1.0f;
    statsWeights_[STATS_TYPE_MELEE_DPS] = 0.01f;
    statsWeights_[STATS_TYPE_RANGED_DPS] = 0.01f;

    if (cls_ == CLASS_WARRIOR)
    {
        statsWeights_[STATS_TYPE_STRENGTH] = 2.5f;
        statsWeights_[STATS_TYPE_AGILITY] = 0.8f;
        statsWeights_[STATS_TYPE_HIT] = 2.0f;
        statsWeights_[STATS_TYPE_CRIT] = 2.0f;
        statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
        statsWeights_[STATS_TYPE_MELEE_DPS] = 7.0f;
    }
    else if (cls_ == CLASS_PALADIN)
    {
        statsWeights_[STATS_TYPE_STRENGTH] = 2.0f;
        statsWeights_[STATS_TYPE_STAMINA] = 0.5f;
        statsWeights_[STATS_TYPE_INTELLECT] = 0.5f;
        statsWeights_[STATS_TYPE_SPIRIT] = 0.3f;
        statsWeights_[STATS_TYPE_HIT] = 1.5f;
        statsWeights_[STATS_TYPE_CRIT] = 1.5f;
        statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
        statsWeights_[STATS_TYPE_MELEE_DPS] = 5.0f;
    }
    else if (cls_ == CLASS_ROGUE)
    {
        statsWeights_[STATS_TYPE_AGILITY] = 2.5f;
        statsWeights_[STATS_TYPE_STRENGTH] = 1.0f;
        statsWeights_[STATS_TYPE_HIT] = 2.0f;
        statsWeights_[STATS_TYPE_CRIT] = 2.0f;
        statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
        statsWeights_[STATS_TYPE_MELEE_DPS] = 7.0f;
    }
    else if (cls_ == CLASS_HUNTER)
    {
        statsWeights_[STATS_TYPE_AGILITY] = 2.5f;
        statsWeights_[STATS_TYPE_STRENGTH] = 1.0f;
        statsWeights_[STATS_TYPE_HIT] = 2.0f;
        statsWeights_[STATS_TYPE_CRIT] = 1.5f;
        statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
        statsWeights_[STATS_TYPE_RANGED_DPS] = 10.0f;
    }
    else if (cls_ == CLASS_MAGE)
    {
        statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
        statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
        statsWeights_[STATS_TYPE_HIT] = 1.0f;
        statsWeights_[STATS_TYPE_CRIT] = 0.8f;
    }
    else if (cls_ == CLASS_WARLOCK)
    {
        statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
        statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
        statsWeights_[STATS_TYPE_HIT] = 1.0f;
        statsWeights_[STATS_TYPE_CRIT] = 0.8f;
    }
    else if (cls_ == CLASS_PRIEST)
    {
        statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
        statsWeights_[STATS_TYPE_SPIRIT] = 1.0f;
        statsWeights_[STATS_TYPE_HIT] = 1.0f;
        statsWeights_[STATS_TYPE_CRIT] = 0.8f;
    }
    else if (cls_ == CLASS_SHAMAN)
    {
        statsWeights_[STATS_TYPE_STRENGTH] = 1.5f;
        statsWeights_[STATS_TYPE_INTELLECT] = 1.0f;
        statsWeights_[STATS_TYPE_SPIRIT] = 0.5f;
        statsWeights_[STATS_TYPE_HIT] = 1.5f;
        statsWeights_[STATS_TYPE_CRIT] = 1.5f;
        statsWeights_[STATS_TYPE_ATTACK_POWER] = 1.0f;
        statsWeights_[STATS_TYPE_MELEE_DPS] = 5.0f;
    }
    else if (cls_ == CLASS_DRUID)
    {
        statsWeights_[STATS_TYPE_STRENGTH] = 2.0f;
        statsWeights_[STATS_TYPE_AGILITY] = 1.5f;
        statsWeights_[STATS_TYPE_STAMINA] = 0.5f;
        statsWeights_[STATS_TYPE_INTELLECT] = 0.5f;
        statsWeights_[STATS_TYPE_SPIRIT] = 0.3f;
        statsWeights_[STATS_TYPE_HIT] = 2.0f;
        statsWeights_[STATS_TYPE_CRIT] = 1.5f;
        statsWeights_[STATS_TYPE_MELEE_DPS] = 5.0f;
    }
    else if (cls_ == CLASS_WARLOCK)
    {
        statsWeights_[STATS_TYPE_INTELLECT] = 2.0f;
        statsWeights_[STATS_TYPE_SPIRIT] = 0.8f;
        statsWeights_[STATS_TYPE_HIT] = 1.0f;
        statsWeights_[STATS_TYPE_CRIT] = 0.8f;
    }
}
