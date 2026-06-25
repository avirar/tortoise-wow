#ifndef _PLAYERBOT_STATSWEIGHTCALCULATOR_H
#define _PLAYERBOT_STATSWEIGHTCALCULATOR_H

#include "StatsCollector.h"
#include "ObjectGuid.h"

class Player;

class StatsWeightCalculator
{
public:
    StatsWeightCalculator(Player* player);
    void Reset();
    float CalculateItem(uint32 itemId, int32 randomPropertyId = 0);

private:
    void GenerateWeights(Player* player);

    Player* player_;
    CollectorType type_;
    StatsCollector* collector_;
    uint8 cls_;
    uint8 lvl_;

    float weight_;
    float statsWeights_[STATS_TYPE_MAX];
};

#endif
