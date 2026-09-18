/*
 *  BotStateSnapshot.cpp — JSON snapshots for the R6.1 agent interface.
 *  Nearby-scan reuses the ServerFacade CellImpl visitor idiom (range-filtered,
 *  capped). Inventory uses the flat top-level slots: 0-18 equipment, 19-22
 *  bags, 23-38 pack. Quests from the public getQuestStatusMap().
 */

#include "BotStateSnapshot.h"
#include "BotCommandAPI.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "Item.h"
#include "Maps/Map.h"
#include "Maps/CellImpl.h"
#include "Bot/PlayerbotAIBase.h"
#include "PlayerBotMgr.h"
#include "PlayerBotAI.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "PlayerQuestMgr.h"
#include "Logging.h"
#include "Log.h"
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <time.h>

namespace
{
    // mangosd CWD is server/bin; live logs live in server/logs (LogFile
// is resolved relative to the server dir). ../logs from bin/.
static const char* kBotStateDir = "../logs/botstate";

    // This core has no StringToNumber (AC-only) — local converters.
    inline std::string Num(uint32 v) { return std::to_string(v); }
    inline std::string Num(int v) { return std::to_string(v); }
    inline std::string Num(uint8 v) { return std::to_string((unsigned)v); }
    inline std::string Num(double v, int prec = 1)
    {
        std::ostringstream o;
        o << std::fixed << std::setprecision(prec) << v;
        return o.str();
    }

    std::string JsonEscOuter(const char* s)
    {
        std::string r;
        for (const char* c = s; c && *c; ++c)
        {
            switch (*c)
            {
                case '"': r += "\\\""; break;
                case '\\': r += "\\\\"; break;
                case '\n': r += "\\n"; break;
                case '\t': r += "\\t"; break;
                default:
                    if (*c >= 0x20)
                        r += *c;
            }
        }
        return r;
    }

    std::string EngineStr(BotState s)
    {
        switch (s)
        {
            case BOT_STATE_COMBAT: return "combat";
            case BOT_STATE_NON_COMBAT: return "noncombat";
            case BOT_STATE_DEAD: return "dead";
            default: return "unknown";
        }
    }

    // Cell-scan visitor (file scope: local classes can't hold member
    // templates in C++ — same reason ServerFacade's visitor is named).
    struct NearbySnapshotVisitor
    {
        Player* bot;
        std::string* out;
        int count;
        const float RANGE = 40.0f;
        const int MAXC = 15;

        void AddCreature(Creature* c)
        {
            if (!c || count >= MAXC)
                return;
            float d = bot->GetDistance2d(c);
            if (d > RANGE)
                return;
            ++count;
            bool hostile = !bot->IsFriendlyTo(c);
            *out += (count > 1 ? "," : "");
            *out += std::string("{\"entry\":") + Num(c->GetEntry())
                + ",\"guid\":" + Num(c->GetObjectGuid().GetCounter())
                + ",\"level\":" + Num(c->GetLevel())
                + ",\"name\":\"" + JsonEscOuter(c->GetName() ? c->GetName() : "") + "\""
                + ",\"dist\":" + Num(d, 1)
                + ",\"alive\":" + (c->IsAlive() ? "true" : "false")
                + ",\"hostile\":" + (hostile ? "true" : "false") + "}";
        }

        void AddPlayer(Player* p)
        {
            if (!p || p == bot || count >= MAXC)
                return;
            float d = bot->GetDistance2d(p);
            if (d > RANGE || d <= 0.0f)
                return;
            ++count;
            *out += (count > 1 ? "," : "");
            *out += std::string("{\"player\":\"") + JsonEscOuter(p->GetName() ? p->GetName() : "")
                + "\",\"level\":" + Num(p->GetLevel())
                + ",\"gm\":" + (p->IsGameMaster() ? "true" : "false")
                + ",\"dist\":" + Num(d, 1) + "}";
        }

        void Visit(PlayerMapType& m)
        {
            for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                AddPlayer(itr->getSource());
        }
        void Visit(CreatureMapType& m)
        {
            for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                AddCreature(itr->getSource());
        }
        template<class T> void Visit(GridRefManager<T>&)
        {
        }
        template<class T> void Visit(T&)
        {
        }
    };

    std::string ClassStr(uint8 c)
    {
        static const char* names[9] = { "Warrior", "Paladin", "Hunter", "Rogue",
            "Priest", "Shaman", "Mage", "Warlock", "Druid" };
        return (c >= 1 && c <= 9) ? names[c - 1] : "Unknown";
    }

    std::string RaceStr(uint8 r)
    {
        static const char* names[10] = { "Human", "Orc", "Dwarf", "NightElf",
            "Undead", "Tauren", "Gnome", "Troll", "Goblin", "BloodElf" };
        return (r >= 1 && r <= 10) ? names[r - 1] : "Unknown";
    }

    void AppendNearby(std::string& out, Player* bot)
    {
        Map* map = bot->GetMap();
        if (!map)
            return;

        NearbySnapshotVisitor v;
        v.bot = bot;
        v.out = &out;
        v.count = 0;

        CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
        Cell cell(p);
        cell.SetNoCreate();
        TypeContainerVisitor<NearbySnapshotVisitor, WorldTypeMapContainer> world_vis(v);
        TypeContainerVisitor<NearbySnapshotVisitor, GridTypeMapContainer> grid_vis(v);
        cell.Visit(p, world_vis, *map, *bot, v.RANGE);
        cell.Visit(p, grid_vis, *map, *bot, v.RANGE);
    }

    void AppendInventory(std::string& out, Player* bot)
    {
        const uint8 slots[3] = { EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND, EQUIPMENT_SLOT_RANGED };
        const char* slotNames[3] = { "mainhand", "offhand", "ranged" };
        for (int i = 0; i < 3; ++i)
        {
            Item* it = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slots[i]);
            if (!it)
                continue;
            ItemPrototype const* proto = it->GetProto();
            out += std::string("\"") + slotNames[i] + "\":{"
                + std::string("\"entry\":") + Num(it->GetEntry())
                + ",\"ilvl\":" + Num(proto->ItemLevel)
                + ",\"name\":\"" + JsonEscOuter(proto->Name1.c_str()) + "\"}";
            out += ",";
        }

        int bagSlots = 0;
        int packSlots = 0;
        for (uint8 s = INVENTORY_SLOT_BAG_START; s < INVENTORY_SLOT_BAG_END; ++s)
            if (bot->GetItemByPos(INVENTORY_SLOT_BAG_0, s))
                ++bagSlots;
        for (uint8 s = INVENTORY_SLOT_ITEM_START; s < INVENTORY_SLOT_ITEM_END; ++s)
            if (bot->GetItemByPos(INVENTORY_SLOT_BAG_0, s))
                ++packSlots;
        out += std::string("\"bagSlots\":") + Num(bagSlots)
            + ",\"packSlots\":" + Num(packSlots);
    }

    void AppendQuests(std::string& out, Player* bot)
    {
        int n = 0;
        QuestStatusMap const* qmap = &const_cast<Player*>(bot)->getQuestStatusMap();
        for (QuestStatusMap::const_iterator itr = qmap->begin(); itr != qmap->end(); ++itr)
        {
            QuestStatusData const& qsd = itr->second;
            if (qsd.m_status != QUEST_STATUS_INCOMPLETE && qsd.m_status != QUEST_STATUS_COMPLETE)
                continue;
            if (n >= 8)
                break;
            ++n;
            out += (n > 1 ? "," : "");
            out += std::string("\"") + Num(itr->first) + "\":{"
                + std::string("\"status\":\"") + (qsd.m_status == QUEST_STATUS_COMPLETE ? "complete" : "in_progress") + "\""
                + ",\"rewarded\":" + (qsd.m_rewarded ? "true" : "false") + "}";
        }
    }

    // R7 L3: quest DATA (free slots + nearest giver/taker/objective POIs) —
    // the same PlayerQuestMgr cache the AI context values expose.
    void AppendQuestData(std::string& out, Player* bot)
    {
        std::vector<PlayerQuestMgr::QuestDest> givers, takers, objectives;
        uint8 freeSlots = PlayerQuestMgr::GetFreeQuestLogSlots(bot);
        PlayerQuestMgr::GetActiveTakers(bot, takers);
        PlayerQuestMgr::GetActiveObjectives(bot, objectives);
        PlayerQuestMgr::GetNearbyGivers(bot, givers);

        out += "\"freeSlots\":" + Num(freeSlots)
             + ",\"givers\":" + Num((uint32)givers.size())
             + ",\"takers\":" + Num((uint32)takers.size())
             + ",\"objectives\":" + Num((uint32)objectives.size());
        if (!takers.empty())
        {
            PlayerQuestMgr::QuestDest const& d = takers.front();
            out += ",\"takerNearest\":{\"map\":" + Num(d.map) + ",\"entry\":" + Num(d.entry)
                 + ",\"x\":" + Num(d.x, 1) + ",\"y\":" + Num(d.y, 1) + "}";
        }
        if (!objectives.empty())
        {
            PlayerQuestMgr::QuestDest const& d = objectives.front();
            out += ",\"objectiveNearest\":{\"map\":" + Num(d.map) + ",\"entry\":" + Num(d.entry)
                 + ",\"x\":" + Num(d.x, 1) + ",\"y\":" + Num(d.y, 1) + "}";
        }
        if (!givers.empty())
        {
            PlayerQuestMgr::QuestDest const& d = givers.front();
            out += ",\"giverNearest\":{\"map\":" + Num(d.map) + ",\"entry\":" + Num(d.entry)
                 + ",\"x\":" + Num(d.x, 1) + ",\"y\":" + Num(d.y, 1) + "}";
        }
    }
}

std::string BotStateSnapshot::BuildBotStateJson(Player* bot)
{
    if (!bot)
        return "{}";

    std::ostringstream o;
    o << "{\"identity\":{";
    o << "\"name\":\"" << JsonEscOuter(bot->GetName()) << "\""
        << ",\"guid\":" << Num(bot->GetObjectGuid().GetCounter())
        << ",\"account\":" << Num(bot->GetSession() ? bot->GetSession()->GetAccountId() : 0)
        << ",\"level\":" << Num(bot->GetLevel())
        << ",\"class\":\"" << ClassStr(bot->GetClass()) << "\""
        << ",\"race\":\"" << RaceStr(bot->GetRace()) << "\"},";

    o << "\"location\":{"
        << "\"map\":" << Num(bot->GetMapId())
        << ",\"x\":" << Num(bot->GetPositionX(), 1)
        << ",\"y\":" << Num(bot->GetPositionY(), 1)
        << ",\"z\":" << Num(bot->GetPositionZ(), 1)
        << ",\"area\":" << Num(bot->GetAreaId()) << "},";

    uint32 maxHp = bot->GetMaxHealth() ? bot->GetMaxHealth() : 1;
    uint32 hp = bot->GetHealth();
    uint32 maxMana = bot->GetMaxPower(POWER_MANA) ? bot->GetMaxPower(POWER_MANA) : 1;
    o << "\"resources\":{"
        << "\"hp\":" << Num(hp)
        << ",\"hpPct\":" << Num(hp * 100.0 / maxHp, 1)
        << ",\"mana\":" << Num(bot->GetPower(POWER_MANA))
        << ",\"manaPct\":" << Num(bot->GetPower(POWER_MANA) * 100.0 / maxMana, 1)
        << "},";

    // Combat + engine state (bots only; real players have no AI)
    PlayerBotEntry* e = sPlayerBotMgr.GetBot(bot->GetObjectGuid().GetCounter());
    std::string engineStr;
    std::string targetStr;
    if (e && e->ai)
    {
        PlayerBotAI* ai = e->ai;
        engineStr = EngineStr(ai->engine->GetState());
        Unit* target = ai->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
        if (target && target->IsInWorld())
        {
            std::ostringstream t;
            t << "\"target\":{\"guid\":" << Num(target->GetObjectGuid().GetCounter())
                << ",\"entry\":" << Num(target->GetEntry())
                << ",\"name\":\"" << JsonEscOuter(target->GetName() ? target->GetName() : "")
                << "\",\"level\":" << Num(target->GetLevel())
                << ",\"hp\":" << Num(target->GetHealth()) << "}";
            targetStr = t.str();
        }
    }

    o << "\"combat\":{"
        << "\"alive\":" << (bot->IsAlive() ? "true" : "false")
        << ",\"inCombat\":" << (bot->IsInCombat() ? "true" : "false")
        << ",\"engine\":" << (engineStr.empty() ? "none" : "\"" + engineStr + "\"");
    if (!targetStr.empty())
        o << "," << targetStr;
    o << "},";

    o << "\"nearby\":[";
    std::string near;
    AppendNearby(near, bot);
    o << near << "],";

    o << "\"inventory\":{";
    std::string inv;
    AppendInventory(inv, bot);
    o << inv << "},";

    o << "\"quests\":{";
    std::string quests;
    AppendQuests(quests, bot);
    o << quests << "},";

    o << "\"questdata\":{";
    std::string qd;
    AppendQuestData(qd, bot);
    o << qd << "},";

    // Recent agent commands (BotCommandAPI history, R6.1)
    o << "\"commands\":" << BotCommandAPI::RecentCommandsJson(bot->GetObjectGuid().GetCounter()) << "}";

    return o.str();
}

std::string BotStateSnapshot::BuildBotOneLineJson(Player* bot)
{
    if (!bot)
        return "{}";
    uint32 maxHp = bot->GetMaxHealth() ? bot->GetMaxHealth() : 1;
    std::ostringstream o;
    o << "{\"name\":\"" << JsonEscOuter(bot->GetName()) << "\""
        << ",\"guid\":" << Num(bot->GetObjectGuid().GetCounter())
        << ",\"lvl\":" << Num(bot->GetLevel())
        << ",\"cls\":\"" << ClassStr(bot->GetClass()) << "\""
        << ",\"map\":" << Num(bot->GetMapId())
        << ",\"x\":" << Num(bot->GetPositionX(), 0)
        << ",\"y\":" << Num(bot->GetPositionY(), 0)
        << ",\"hp\":" << Num(bot->GetHealth() * 100.0 / maxHp, 0)
        << ",\"alive\":" << (bot->IsAlive() ? "true" : "false")
        << ",\"combat\":" << (bot->IsInCombat() ? "true" : "false");
    PlayerBotEntry* e = sPlayerBotMgr.GetBot(bot->GetObjectGuid().GetCounter());
    if (e && e->ai)
        o << ",\"engine\":\"" << EngineStr(e->ai->engine->GetState()) << "\"";
    o << "}";
    return o.str();
}

std::string BotStateSnapshot::BuildBotsListJson()
{
    std::ostringstream o;
    o << "[";
    bool first = true;
    for (std::map<uint32, PlayerBotEntry*>::const_iterator i = sPlayerBotMgr.GetBotsMap().begin();
         i != sPlayerBotMgr.GetBotsMap().end(); ++i)
    {
        PlayerBotEntry* e = i->second;
        if (!e || e->state != PB_STATE_ONLINE || !e->ai)
            continue;
        Player* bot = e->ai->me;
        if (!bot || !bot->IsInWorld())
            continue;
        if (!first)
            o << ",";
        first = false;
        o << BuildBotOneLineJson(bot);
    }
    o << "]";
    return o.str();
}

void BotStateSnapshot::DumpStateFile()
{
    std::string mdc = std::string("mkdir -p ") + kBotStateDir;
    system(mdc.c_str());

    std::string path = std::string(kBotStateDir) + "/bots.json";
    FILE* f = fopen(path.c_str(), "w");
    if (!f)
    {
        sLog.outError("playerbots: botstate: cannot open %s", path.c_str());
        return;
    }
    fprintf(f, "%s\n", BuildBotsListJson().c_str());
    fclose(f);
}

void BotStateSnapshot::DumpBotFile(Player* bot)
{
    std::string mdc = std::string("mkdir -p ") + kBotStateDir;
    system(mdc.c_str());

    std::string path = std::string(kBotStateDir) + "/" + JsonEscOuter(bot->GetName()) + ".json";
    FILE* f = fopen(path.c_str(), "w");
    if (!f)
    {
        sLog.outError("playerbots: botstate: cannot open %s", path.c_str());
        return;
    }
    fprintf(f, "%s\n", BuildBotStateJson(bot).c_str());
    fclose(f);
}
