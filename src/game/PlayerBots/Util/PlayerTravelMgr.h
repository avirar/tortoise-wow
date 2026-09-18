/*
 * PlayerTravelMgr — R5e: real population travel (AC RandomPlayerbotMgr pattern)
 *
 * Population bots get periodic random travel every 1-5h (AC
 * min/maxRandomBotTeleportInterval): pick a level-appropriate hub from REAL
 * innkeeper/flightmaster/banker NPC positions in the world DB, 25% chance to
 * a city/banker which also updates the homebind. This is the "real" travel
 * layer — the idle-relocation sweep (grind spots) stays as the emergency net
 * for bots that land somewhere gray.
 *
 * Data (verified against tw_world, 2026-09-17):
 *  - hubs: creature + creature_template, npc_flags & 392 (FLIGHTMASTER=8 |
 *    INNKEEPER=128 | BANKER=256 in this core), maps 0/1 only.
 *  - level: the NPC's own level_min (town-level proxy): 10=Elwynn (Tiriel
 *    Brightwater), 20=Southshore (Brundah Cliffbrow), 48x lvl-30 mid towns,
 *    81x lvl-55 cities/high hubs.
 *  - faction: faction_template.hostile_mask (bit 4 = hostile to horde,
 *    bit 2 = hostile to alliance — verified empirically).
 *  - ground z: Map::GetHeight at the landing point (AC: z = 0.05 + ground).
 */
#ifndef _PLAYERBOT_TRAVEL_MGR_H
#define _PLAYERBOT_TRAVEL_MGR_H

#include "Common.h"
#include <vector>

class Player;

class PlayerTravelMgr
{
public:
    struct Hub
    {
        uint32 map;
        float x, y, z;
        uint16 entry;
        uint8  level;    // NPC level (town-level proxy; 55+ = city/high hub)
        uint8  inn, flight, banker;
        uint32 factionId;
        bool   city;     // level >= 55 → homebind candidate
    };

    // One-time lazy load of hub positions from tw_world (~200 rows).
    static void EnsureHubsLoaded();

    // One-time lazy load of quest-POI cache (creature_questrelation ⋈
    // creature ⋈ quest_template, overworld maps 0/1, ~6k rows pre-dedupe):
    // every NPC linked to a quest, with the quest's level span + primary
    // kill target (ReqCreatureOrGOId1). User-directed R5e extension: cache
    // quest givers and objectives at startup for travel/POI lookups (no
    // pfQuest import — the server's own quest data is self-contained).
    struct QuestPoi
    {
        uint32 map;
        float x, y, z;
        uint16 entry;       // quest NPC entry (giver/turn-in)
        uint32 quest;       // a linked quest (first seen)
        uint32 minLevel;    // quest MinLevel span (0 = uncapped)
        uint32 maxLevel;    // quest MaxLevel span (0 = uncapped)
        uint32 killEntry;   // primary kill target (ReqCreatureOrGOId1, 0 = n/a)
    };
    static void EnsureQuestPoisLoaded();
    static bool QuestPoiLevelMatch(uint32 botLvl, QuestPoi const& p);

    // Level + faction match: can `bot` use this hub?
    static bool IsUsableFor(Player const* bot, Hub const& h);

    // Picked travel destination (a TOWN — inn/flight/bank/quest NPC). Never
    // a hostile mob's spawn: R5e.2 retired mob-anchored relocation after the
    // night-verify (lvl-12 bots dropped into Venture Co lvl-14-17 camps).
    struct TravelDest
    {
        uint32 map;
        float x, y, z;
        bool city;
        uint16 entry;
        const char* reason; // "city" | "quest poi" | "hub"
    };

    // Level/faction-matched destination pick with the standard mix (25% city
    // for lvl 10+, else 50/50 quest-POI vs inn/flight/bank hub). Shared by
    // DoTravel AND the emergency sweeps (idle relocation + stale-combat
    // breaker) — the emergency net re-travels to a town, not a mob.
    static bool PickDestination(Player const* bot, TravelDest& dest, bool allowCity = true);

    // Anti-cam (AC): a real (non-bot) player within range → no visible blinks.
    static bool HasRealPlayerNearby(Player const* bot, float range);

    // Full travel: guards → hub pick (25% city/homebind) → teleport.
    // Returns true when the bot was teleported.
    static bool DoTravel(Player* bot);

private:
    static std::vector<Hub> s_hubs;
    static bool s_hubsLoaded;
    static std::vector<QuestPoi> s_questPois;
    static bool s_questPoisLoaded;
};

#endif
