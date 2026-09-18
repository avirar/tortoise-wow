/*
 * PlayerGrindMgr — R7 L2 grind-position cache. See header for the design.
 */

#include "PlayerGrindMgr.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "Maps/MapManager.h"
#include "Maps/Map.h"        // Map::GetHeight (via sMapMgr.FindMap)
#include "GridMap.h"          // INVALID_HEIGHT

std::vector<PlayerGrindMgr::Spot> PlayerGrindMgr::s_spots[2];
bool PlayerGrindMgr::s_loaded = false;

void PlayerGrindMgr::EnsureLoaded()
{
    if (s_loaded)
        return;
    s_loaded = true;  // re-entry guard (DB hiccup → empty cache → GO_GRIND
                      // just never picks, the bot idles — not a crash)

    // All NORMAL-rank, level 1-60 spawns on the two continents (~63k rows).
    // Grid-cell-sample in memory: one representative per (1500yd cell × 5-level
    // band) per map → a small, well-spread cache.
    QueryResult* result = WorldDatabase.PQuery(
        "SELECT c.map, c.position_x, c.position_y, c.position_z, t.level_min "
        "FROM creature c JOIN creature_template t ON t.entry = c.id "
        "WHERE c.map IN (0, 1) AND t.rank = 0 AND t.level_min >= 1 AND t.level_min <= 60");

    std::map<uint64, Spot> grid[2];
    uint32 totalRaw = 0;
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            Spot s;
            s.map = f[0].GetUInt32();
            s.x = f[1].GetFloat();
            s.y = f[2].GetFloat();
            s.z = f[3].GetFloat();
            s.level = (uint8)f[4].GetUInt32();
            if (s.map > 1)
                continue;
            ++totalRaw;
            // int truncation toward zero is a consistent cell partition
            // (cell 0 spans [-1500,1500)); fine for grid sampling.
            int32 cx = (int32)(s.x / 1500.0f);
            int32 cy = (int32)(s.y / 1500.0f);
            uint32 band = s.level / 5;
            uint64 key = ((uint64)(cx + 1000) << 32) | ((uint64)(cy + 1000) << 16) | (band << 8);
            std::map<uint64, Spot>& g = grid[s.map];
            if (g.find(key) == g.end())
                g[key] = s;
        } while (result->NextRow());
        delete result;
    }

    for (uint32 m = 0; m < 2; ++m)
    {
        for (std::map<uint64, Spot>::iterator i = grid[m].begin(); i != grid[m].end(); ++i)
            s_spots[m].push_back(i->second);
    }

    sLog.outInfo("playerbots: grind spots loaded: %u raw spawns -> %u (map0) + %u (map1) grid-sampled "
                 "(rank0, lvl1-60, 1500yd cells)",
                 totalRaw, (uint32)s_spots[0].size(), (uint32)s_spots[1].size());
}

bool PlayerGrindMgr::SelectRandomGrindPos(Player const* bot, Spot& dest)
{
    uint32 mapId = bot->GetMapId();
    if (mapId > 1)
        return false;
    EnsureLoaded();
    std::vector<Spot> const& v = s_spots[mapId];
    if (v.empty())
        return false;

    int32 lvl = (int32)bot->GetLevel();
    float bx = bot->GetPositionX();
    float by = bot->GetPositionY();

    std::vector<const Spot*> hi, lo;
    for (std::vector<Spot>::const_iterator i = v.begin(); i != v.end(); ++i)
    {
        int32 dl = (int32)i->level - lvl;
        if (dl > 3 || dl < -3)
            continue;
        float dx = i->x - bx;
        float dy = i->y - by;
        float d2 = dx * dx + dy * dy;
        if (d2 > 2500.0f * 2500.0f)
            continue;  // > 2500yd
        if (d2 < 500.0f * 500.0f)
            hi.push_back(&*i);
        else
            lo.push_back(&*i);
    }

    const Spot* pick = 0;
    if (urand(1, 100) <= 50 && !hi.empty())
        pick = hi[urand(0, (uint32)hi.size() - 1)];
    else if (!lo.empty())
        pick = lo[urand(0, (uint32)lo.size() - 1)];

    if (!pick)
        return false;

    dest = *pick;
    // Recompute ground z (spawn z can be stale); fallback to the stored z.
    Map* map = sMapMgr.FindMap(mapId);
    float z = map ? map->GetHeight(dest.x, dest.y, dest.z) : INVALID_HEIGHT;
    if (z <= INVALID_HEIGHT)
        z = dest.z;
    dest.z = z;
    return true;
}

uint32 PlayerGrindMgr::CountForMap(uint32 map)
{
    EnsureLoaded();
    return (map <= 1) ? (uint32)s_spots[map].size() : 0;
}
