/*
 * PlayerTravelMgr — R5e: real population travel (AC RandomPlayerbotMgr pattern)
 * See PlayerTravelMgr.h for the design + verified data sources.
 */
#include "PlayerTravelMgr.h"

#include "Database/DatabaseEnv.h"
#include "Database/DBCStructure.h"
#include "ObjectMgr.h"
#include "World.h"
#include "Maps/MapManager.h"
#include "Maps/Map.h"
#include "Maps/CellImpl.h"
#include "Maps/GridNotifiers.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "MotionMaster.h"
#include "SharedDefines.h"
#include "Spells/SpellAuras.h"
#include "Spells/SpellDefines.h"
#include "PlayerBotMgr.h"
#include "PlayerbotAIConfig.h"
#include "ServerFacade.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "Logging.h"
#include "Log.h"

std::vector<PlayerTravelMgr::Hub> PlayerTravelMgr::s_hubs;
bool PlayerTravelMgr::s_hubsLoaded = false;
std::vector<PlayerTravelMgr::QuestPoi> PlayerTravelMgr::s_questPois;
bool PlayerTravelMgr::s_questPoisLoaded = false;

// One-time load: real innkeeper/flightmaster/banker positions on the
// overworld continents. Our core's NPC flags: FLIGHTMASTER=8, INNKEEPER=128,
// BANKER=256 → mask 392.
void PlayerTravelMgr::EnsureHubsLoaded()
{
    if (s_hubsLoaded)
        return;
    s_hubsLoaded = true; // re-entry guard (DB hiccup → empty list → no travel,
                         // the idle-relocation sweep still self-heals)

    QueryResult* result = WorldDatabase.PQuery(
        "SELECT c.map, c.position_x, c.position_y, c.position_z, c.id, "
        "t.level_min, t.npc_flags, t.faction "
        "FROM creature c JOIN creature_template t ON t.entry = c.id "
        "WHERE (t.npc_flags & 392) AND c.map IN (0, 1)");

    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            Hub h;
            h.map = f[0].GetUInt32();
            h.x = f[1].GetFloat();
            h.y = f[2].GetFloat();
            h.z = f[3].GetFloat();
            h.entry = f[4].GetUInt32();
            h.level = (uint8)f[5].GetUInt32();
            uint32 nf = f[6].GetUInt32();
            h.flight = (nf & 8) ? 1 : 0;
            h.inn = (nf & 128) ? 1 : 0;
            h.banker = (nf & 256) ? 1 : 0;
            h.factionId = f[7].GetUInt32();
            // 55/56 = the core's default level for city/high hubs (verified:
            // 81 NPCs at 55) — treat as cities (homebind candidates, 40+).
            h.city = (h.level >= 55);
            s_hubs.push_back(h);
        } while (result->NextRow());
        delete result;
    }
    sLog.outInfo("playerbots: travel hubs loaded: %u innkeeper/flight/bank locations", (uint32)s_hubs.size());
}

// Quest-POI cache: every overworld NPC linked to a quest (creature_quest
// relation ⋈ creature ⋈ quest_template). One row per (entry, map) with the
// union level span + primary kill target. One-time startup cost (~6k rows).
void PlayerTravelMgr::EnsureQuestPoisLoaded()
{
    if (s_questPoisLoaded)
        return;
    s_questPoisLoaded = true; // re-entry guard (same rationale as hubs)

    QueryResult* result = WorldDatabase.PQuery(
        "SELECT c.map, c.position_x, c.position_y, c.position_z, c.id, "
        "cqr.quest, q.MinLevel, q.MaxLevel, q.ReqCreatureOrGOId1 "
        "FROM creature c "
        "JOIN creature_questrelation cqr ON cqr.id = c.id "
        "JOIN quest_template q ON q.entry = cqr.quest "
        "WHERE c.map IN (0, 1)");

    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            QuestPoi p;
            p.map = f[0].GetUInt32();
            p.x = f[1].GetFloat();
            p.y = f[2].GetFloat();
            p.z = f[3].GetFloat();
            p.entry = (uint16)f[4].GetUInt32();
            p.quest = f[5].GetUInt32();
            uint32 lo = f[6].GetUInt32();
            uint32 hi = f[7].GetUInt32();
            p.killEntry = f[8].GetUInt32();
            p.minLevel = lo;  // R5e.2 BUGFIX: first-push left these as stack
            p.maxLevel = hi;  // garbage (often 0 = "uncapped" → ANY bot level
                               // matched → lvl-6 bot at a lvl-50 quest POI)

            // Dedupe by (entry, map): merge level spans (an NPC with several
            // quests covers the union of their levels).
            bool merged = false;
            for (std::vector<QuestPoi>::iterator i = s_questPois.begin(); i != s_questPois.end(); ++i)
            {
                if (i->entry == p.entry && i->map == p.map)
                {
                    if (lo != 0 && (i->minLevel == 0 || lo < i->minLevel))
                        i->minLevel = lo;
                    if (hi > i->maxLevel)
                        i->maxLevel = hi;
                    if (i->killEntry == 0)
                        i->killEntry = p.killEntry;
                    merged = true;
                    break;
                }
            }
            if (!merged)
                s_questPois.push_back(p);
        } while (result->NextRow());
        delete result;
    }
    sLog.outInfo("playerbots: quest POI cache loaded: %u quest NPCs (overworld)", (uint32)s_questPois.size());
}

bool PlayerTravelMgr::QuestPoiLevelMatch(uint32 botLvl, QuestPoi const& p)
{
    // Quest level span with ±2 tolerance (tightened from ±3 at night verify:
    // a lvl-12 bot landing at a lvl-15 POI found only +3..+10 ambient mobs —
    // gray under MaxTargetLevelDiff=4). minLevel 0 = no floor, maxLevel 0
    // = no ceiling (both common in this data).
    uint32 lo = p.minLevel == 0 ? 1 : (p.minLevel > 2 ? p.minLevel - 2 : 1);
    if (botLvl < lo)
        return false;
    if (p.maxLevel != 0)
    {
        if (botLvl > p.maxLevel + 2)
            return false;
        // R5e.2: a very wide quest span (e.g. min 5 → max 45) means a
        // mixed-level giver — the ambient-mob level proxy is unreliable there.
        if (p.maxLevel - p.minLevel > 15)
            return false;
    }
    else if (p.minLevel != 0 && botLvl > p.minLevel + 8)
    {
        // R5e.2: MaxLevel=0 = "uncapped" (very common in this data). For the
        // zone-proxy purpose that's too loose: a MinLevel=1 giver in a lvl-1-5
        // zone matched ANY level (a lvl-11 bot landed in a starting zone).
        // Treat uncapped as a minLevel+8 window (a typical quest span).
        return false;
    }
    return true;
}

bool PlayerTravelMgr::IsUsableFor(Player const* bot, Hub const& h)
{
    uint32 botLvl = bot->GetLevel();

    // Level bracket: NPC level is a town-level proxy (10=Elwynn, 20=
    // Southshore, 30=mid towns, 55+=cities). Tight band (night verify:
    // -10/+5 landed bots in zones whose ambient mobs were +5..+10 — all
    // gray under MaxTargetLevelDiff=4 — then stuck in town combat/idle
    // loops). A bot a bit out-of-band is fixed within one relocation grace.
    if (h.city)
    {
        if (botLvl < 40)
            return false;
    }
    else
    {
        uint32 lo = h.level > 5 ? h.level - 5 : 1;
        if (botLvl < lo || botLvl > h.level + 3)
            return false;
    }

    // Faction: don't walk a Horde bot into Stormwind's inn (and vice versa).
    // faction_template.hostile_mask: bit 4 = hostile to horde, bit 2 =
    // hostile to alliance (verified empirically against Tiriel/Brax Goldgrasp).
    FactionTemplateEntry const* fe = sObjectMgr.GetFactionTemplateEntry(h.factionId);
    if (fe)
    {
        if ((fe->hostileMask & 4) && bot->GetTeamId() == TEAM_HORDE)
            return false;
        if ((fe->hostileMask & 2) && bot->GetTeamId() == TEAM_ALLIANCE)
            return false;
    }
    return true;
}

// Visitor: any REAL (non-bot) player within range? (AC anti-cam check —
// no visible blinks in front of players)
struct RealPlayerInCellCollector
{
    Player const* me;
    float range;
    float range2;
    bool found;

    RealPlayerInCellCollector() : me(nullptr), range(0), range2(0), found(false) {}

    void Init(Player const* m, float r)
    {
        me = m;
        range = r;
        range2 = r * r;
        found = false;
    }

    void Visit(PlayerMapType &m)
    {
        for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            Player* pl = itr->getSource();
            if (!pl || pl == me || !pl->IsInWorld() || !pl->IsAlive())
                continue;
            if (me->GetDistance2d(pl) > range)
                continue;
            // A bot? sPlayerBotMgr knows every bot guid.
            if (!sPlayerBotMgr.GetBot(pl->GetObjectGuid().GetCounter()))
                found = true; // real player
        }
    }

    void Visit(CreatureMapType &) {}
    template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    template<class NOT_INTERESTED> void Visit(NOT_INTERESTED &) {}
};

bool PlayerTravelMgr::HasRealPlayerNearby(Player const* bot, float range)
{
    Map* map = bot->GetMap();
    if (!map)
        return false;
    RealPlayerInCellCollector collector;
    collector.Init(bot, range);

    CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();

    TypeContainerVisitor<RealPlayerInCellCollector, WorldTypeMapContainer> world_vis(collector);
    TypeContainerVisitor<RealPlayerInCellCollector, GridTypeMapContainer> grid_vis(collector);

    cell.Visit(p, world_vis, *map, *bot, range);
    cell.Visit(p, grid_vis, *map, *bot, range);
    return collector.found;
}

bool PlayerTravelMgr::PickDestination(Player const* bot, TravelDest& dest, bool allowCity)
{
    // AC RandomTeleportForLevel mix: lvl 10+ has a 25% chance to travel to a
    // city/banker (and refresh the homebind there). Otherwise 50/50 between
    // a level-matched QUEST POI (giver/turn-in NPC — the quest MinLevel is a
    // finer zone-level proxy than the inn NPC's level) and a plain
    // inn/flight/bank hub.
    bool wantCity = false;
    if (allowCity && bot->GetLevel() >= 10 && sPlayerbotAIConfig.travelCityChancePct > 0 &&
        urand(0, 99) < sPlayerbotAIConfig.travelCityChancePct)
        wantCity = true;

    dest.city = false;
    dest.entry = 0;

    if (wantCity)
    {
        EnsureHubsLoaded();
        std::vector<Hub const*> cities;
        for (std::vector<Hub>::const_iterator i = s_hubs.begin(); i != s_hubs.end(); ++i)
            if (i->city && IsUsableFor(bot, *i))
                cities.push_back(&*i);
        if (cities.empty())
            wantCity = false; // no city for this level → local dest
        else
        {
            Hub const& h = *cities[urand(0, (uint32)cities.size() - 1u)];
            dest.map = h.map; dest.x = h.x; dest.y = h.y; dest.z = h.z;
            dest.city = true; dest.entry = h.entry; dest.reason = "city";
            return true;
        }
    }
    if (!wantCity)
    {
        bool usePoi = sPlayerbotAIConfig.travelPoiChancePct > 0 &&
            urand(0, 99) < sPlayerbotAIConfig.travelPoiChancePct;
        if (usePoi)
        {
            EnsureQuestPoisLoaded();
            std::vector<QuestPoi const*> pois;
            for (std::vector<QuestPoi>::const_iterator i = s_questPois.begin(); i != s_questPois.end(); ++i)
                if (QuestPoiLevelMatch(bot->GetLevel(), *i))
                    pois.push_back(&*i);
            if (pois.empty())
                usePoi = false; // no POI for this level → plain hub
            else
            {
                QuestPoi const& p = *pois[urand(0, (uint32)pois.size() - 1u)];
                dest.map = p.map; dest.x = p.x; dest.y = p.y; dest.z = p.z;
                dest.city = false; dest.entry = p.entry; dest.reason = "quest poi";
                return true;
            }
        }
        if (!usePoi)
        {
            EnsureHubsLoaded();
            std::vector<Hub const*> hubs;
            for (std::vector<Hub>::const_iterator i = s_hubs.begin(); i != s_hubs.end(); ++i)
                if (!i->city && i->level > 5 && IsUsableFor(bot, *i)) // R5e.2: level 1-5 "hubs" are wilderness caravan/mount NPCs (e.g. Caravan Kodo lvl-1 in lvl-8-12 Westfall)
                    hubs.push_back(&*i);
            if (hubs.empty())
                return false;
            Hub const& h = *hubs[urand(0, (uint32)hubs.size() - 1u)];
            dest.map = h.map; dest.x = h.x; dest.y = h.y; dest.z = h.z;
            dest.city = false; dest.entry = h.entry; dest.reason = "hub";
            return true;
        }
    }
    return false;
}

bool PlayerTravelMgr::DoTravel(Player* bot)
{
    if (!bot || !bot->IsInWorld() || !bot->IsAlive())
        return false;
    // Guards (AC RandomTeleport): never yank a bot out of a fight, a
    // teleport in flight, a BG/arena, or a group (grouped bots move as a
    // unit — R4).
    if (bot->IsInCombat() || bot->IsBeingTeleported() || bot->IsRooted())
        return false;
    if (bot->InBattleGround() || bot->InArena() || bot->InBattleGroundQueue())
        return false;
    if (bot->GetGroup())
        return false;
    Map* curMap = bot->GetMap();
    if (!curMap || curMap->IsDungeon())
        return false;

    // Anti-cam: defer when a real player is nearby (AC: break the loop).
    if (HasRealPlayerNearby(bot, 150.0f))
        return false;

    // Destination pick (shared with the emergency sweeps — R5e.2: the
    // relocation/stale-combat net re-travels to a TOWN, never a hostile
    // mob's spawn; mob-anchored relocation retired after the night-verify).
    TravelDest dest;
    if (!PickDestination(bot, dest))
        return false;

    uint32 destMap = dest.map;
    float destX = dest.x, destY = dest.y, destZ = dest.z;
    bool destCity = dest.city;
    uint16 destEntry = dest.entry;
    const char* reason = dest.reason;


    float x = destX + (float)((int)urand(0, 60) - 30);
    float y = destY + (float)((int)urand(0, 60) - 30);
    float z = destZ;
    Map* targetMap = sMapMgr.FindMap(destMap);
    if (!targetMap)
        return false;
    // Ground truth at the jittered point (AC: z = 0.05 + ground).
    float ground = targetMap->GetHeight(x, y, z);
    z = (ground <= INVALID_HEIGHT) ? (z + 0.05f) : (ground + 0.05f);

    if (!MapManager::IsValidMapCoord(destMap, x, y, z))
        return false;

    // City/banker travel refreshes the homebind (AC: SetHomebind(loc, zone))
    // — the population's revive anchor follows where it actually lives.
    if (destCity)
    {
        WorldLocation loc(destMap, x, y, z, 0.0f);
        // This core's Map is a GridRefManager, not a GridMap — the area id
        // comes from the terrain manager (GridMap.h:227).
        uint32 areaId = sTerrainMgr.GetAreaId(destMap, x, y, z);
        bot->SetHomebindToLocation(loc, areaId);
    }

    // AC teleport sequence: clear motion, clear the stale "current target"
    // (our Engine::Reset cannot be used — it deletes strategies/triggers;
    // the shared-context clear is the safe equivalent of DropTarget),
    // drop teleport-interruptible auras, then teleport.
    bot->GetMotionMaster()->Clear();
    if (PlayerBotEntry* e = sPlayerBotMgr.GetBot(bot->GetObjectGuid().GetCounter()))
        if (e->ai)
            if (AiObjectContext* ctx = e->ai->GetAiObjectContext())
                ctx->GetValue<Unit*>("current target")->Set(nullptr);
    bot->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED | AURA_INTERRUPT_FLAG_CHANGE_MAP);

    if (!bot->TeleportTo(destMap, x, y, z, 0.0f))
        return false;

    ServerFacade::MarkViableGrindTargetSeen(bot); // fresh relocation grace

    sLog.outInfo("playerbots: traveled bot %s (lvl %u) to %s %u (map %u %.0f,%.0f,%.0f)%s",
        bot->GetName(), bot->GetLevel(), reason, destEntry, destMap, x, y, z,
        destCity ? " [homebind]" : "");
    return true;
}
