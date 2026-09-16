#ifndef _PLAYERBOT_STATSWEIGHTCALCULATOR_H
#define _PLAYERBOT_STATSWEIGHTCALCULATOR_H

#include "StatsCollector.h"
#include "Common.h"

class Player;

// Vanilla hit caps (raw percentages)
enum HitCap
{
    MELEE_HIT_CAP = 3,    // 3% to hit level 63 from level 60
    SPELL_HIT_CAP = 17,   // 17% to hit spells (no dodge in vanilla)
    RANGED_HIT_CAP = 3    // Same as melee
};

class StatsWeightCalculator
{
public:
    StatsWeightCalculator(Player* player);
    ~StatsWeightCalculator() { delete collector_; }
    void Reset();
    float CalculateItem(uint32 itemId, int32 randomPropertyId = 0);

    // Role detection (vanilla: class-based, no formal spec system)
    static bool IsMelee(Player* player);
    static bool IsRanged(Player* player);
    static bool IsCaster(Player* player);
    static bool IsHealer(Player* player);
    static bool IsTank(Player* player);

    // Get collector type for this player
    CollectorType GetCollectorType() const { return type_; }

private:
    void GenerateWeights(Player* player);

    Player* player_;
    CollectorType type_;
    StatsCollector* collector_;
    uint8 cls_;
    uint8 lvl_;

    float weight_;
    float statsWeights_[STATS_TYPE_MAX];

    // Current hit/crit totals from equipped gear (for overflow calculation)
    float currentHit_;
    float currentCrit_;
};

#endif
