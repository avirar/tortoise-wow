/*
 * PlayerGrindMgr — R7 L2: level-band grind-position cache.
 *
 * Faithful adaptation of AC mod-playerbots `sTravelMgr.GetLocsPerLevelCache(level)`
 * (the grind-position source for `NewRpgBaseAction::SelectRandomGrindPos`). AC's
 * sTravelMgr is a WotLK module cache; this core has no equivalent, so we build the
 * same data from the world DB's own creature spawns: positions of NORMAL-rank
 * (rank=0), level 1-60 mobs on the two continents (maps 0/1).
 *
 * The raw set is ~63k spawns (too many to linear-scan per query), so we
 * grid-cell-sample it at load: one representative spawn per (1500yd cell ×
 * 5-level band) per map. Result: a small, geographically-well-spread cache
 * (~1-3k entries/map) where `SelectRandomGrindPos` does a cheap same-map scan
 * with a 2500yd range + level-band filter — matching AC's hi(<500yd)/lo(<2500yd)
 * pick exactly.
 *
 * This is the GO_GRIND destination source: the bot WALKS here (MoveFarTo) and the
 * existing grind layer (SelectNearestSafeTarget) kills the mobs that are actually
 * there. A grind spot is where mobs ARE, not a town — so this is intentionally
 * separate from PlayerTravelMgr (towns/hubs for travel + homebind).
 */
#ifndef _PLAYERBOT_GRIND_MGR_H
#define _PLAYERBOT_GRIND_MGR_H

#include "Common.h"
#include <vector>
#include <map>

class Player;

class PlayerGrindMgr
{
public:
    struct Spot
    {
        uint32 map;
        float x, y, z;
        uint8  level;   // spawn's template level_min
    };

    // One-time lazy load + grid-sample from tw_world (creature ⋈
    // creature_template, rank=0, level 1-60, maps 0/1).
    static void EnsureLoaded();

    // AC SelectRandomGrindPos: same map, within 2500yd (lo) / <500yd (hi),
    // level band bot ±3, 50% chance for a hi (near) spot else a lo spot.
    // Returns true + fills dest when a spot is found.
    static bool SelectRandomGrindPos(Player const* bot, Spot& dest);

    static uint32 CountForMap(uint32 map);

private:
    static std::vector<Spot> s_spots[2];  // index = map (0/1)
    static bool s_loaded;
};

#endif
