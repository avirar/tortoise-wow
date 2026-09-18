/*
 *  BotCommandAPI.cpp — R6.1 agent control surface.
 *  AC reference: mod-ollama-bot-buddy BotBuddyAI (api.cpp) — same command
 *  semantics, adapted to this core's APIs (SetSelectionGuid, SendLoot,
 *  WorldObject::CastSpell, session CMSG handlers for quest replay).
 */

#include "BotCommandAPI.h"
#include "BotStateSnapshot.h"
#include "Bot/PlayerbotAIBase.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "GameObject.h"
#include "Item.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "ObjectAccessor.h"
#include "Maps/Map.h"
#include "Maps/CellImpl.h"
#include "WorldSession.h"
#include "Spells/Spell.h"
#include "PlayerBotMgr.h"
#include "PlayerBotAI.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "Logging.h"
#include "Log.h"
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <deque>
#include <map>
#include <time.h>

namespace
{
    // Per-bot command history (guid -> last 10), read by the snapshot.
    std::map<uint32, std::deque<BotCommandAPI::CmdRec>> g_history;
    const size_t HISTORY_MAX = 10;

    // ---------------------------------------------------------- guid resolution
    // This core's full object guids are NOT the raw low value: creatures carry
    // HIGHGUID_UNIT (0xF130) + entry in the mid part, GOs HIGHGUID_GAMEOBJECT,
    // only players are raw low (HIGHGUID_PLAYER = 0). So a low-guid from the
    // state dump must be resolved by scanning the bot's nearby cells.
    struct CounterObjVisitor
    {
        uint32 counter;
        Unit* center;
        float maxDist;
        Creature* foundCreature;
        GameObject* foundGO;

        void Visit(CreatureMapType& m)
        {
            for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            {
                Creature* c = itr->getSource();
                if (!foundCreature && c && c->IsInWorld()
                    && c->GetObjectGuid().GetCounter() == counter
                    && center->GetDistance2d(c) <= maxDist)
                    foundCreature = c;
            }
        }
        void Visit(GameObjectMapType& m)
        {
            for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            {
                GameObject* go = itr->getSource();
                if (!foundGO && go
                    && go->GetObjectGuid().GetCounter() == counter
                    && center->GetDistance2d(go) <= maxDist)
                    foundGO = go;
            }
        }
        template<class T> void Visit(T&) {}
    };

    // Resolve a low guid (as reported in the state dump) to a nearby
    // creature/GO. Players resolve exactly: HIGHGUID_PLAYER == 0, so the raw
    // low value IS the full player guid.
    Unit* ResolveUnitByCounter(Player* bot, uint32 counter, float maxDist)
    {
        if (Player* p = ObjectAccessor::FindPlayer(ObjectGuid(counter)))
            return p;
        Map* map = bot->GetMap();
        if (!map)
            return nullptr;

        CounterObjVisitor v;
        v.counter = counter;
        v.center = bot;
        v.maxDist = maxDist;
        v.foundCreature = nullptr;
        v.foundGO = nullptr;

        CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
        Cell cell(p);
        cell.SetNoCreate();
        TypeContainerVisitor<CounterObjVisitor, WorldTypeMapContainer> world_vis(v);
        TypeContainerVisitor<CounterObjVisitor, GridTypeMapContainer> grid_vis(v);
        cell.Visit(p, world_vis, *map, *bot, maxDist);
        cell.Visit(p, grid_vis, *map, *bot, maxDist);
        return v.foundCreature;
    }

    GameObject* ResolveGOByCounter(Player* bot, uint32 counter, float maxDist)
    {
        Map* map = bot->GetMap();
        if (!map)
            return nullptr;

        CounterObjVisitor v;
        v.counter = counter;
        v.center = bot;
        v.maxDist = maxDist;
        v.foundCreature = nullptr;
        v.foundGO = nullptr;

        CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
        Cell cell(p);
        cell.SetNoCreate();
        TypeContainerVisitor<CounterObjVisitor, WorldTypeMapContainer> world_vis(v);
        TypeContainerVisitor<CounterObjVisitor, GridTypeMapContainer> grid_vis(v);
        cell.Visit(p, world_vis, *map, *bot, maxDist);
        cell.Visit(p, grid_vis, *map, *bot, maxDist);
        return v.foundGO;
    }

    std::string SpellFailReason(SpellCastResult r)
    {
        switch (r)
        {
            case SPELL_CAST_OK: return "ok";
            case SPELL_FAILED_AFFECTING_COMBAT: return "in combat (cast lockout?)";
            case SPELL_FAILED_BAD_TARGETS: return "invalid target";
            case SPELL_FAILED_BAD_IMPLICIT_TARGETS: return "no target";
            case SPELL_FAILED_CANT_CAST_ON_TAPPED: return "target tapped by other";
            case SPELL_FAILED_EQUIPPED_ITEM: return "wrong equipment";
            case SPELL_FAILED_EQUIPPED_ITEM_CLASS: return "wrong equipment class";
            case SPELL_FAILED_IMMUNE: return "immune";
            case SPELL_FAILED_INTERRUPTED: return "interrupted";
            case SPELL_FAILED_LEVEL_REQUIREMENT: return "level too low";
            case SPELL_FAILED_LINE_OF_SIGHT: return "no line of sight";
            case SPELL_FAILED_NOT_READY: return "cooldown";
            case SPELL_FAILED_NO_POWER: return "not enough power";
            case SPELL_FAILED_TARGETS_DEAD: return "target is dead";
            case SPELL_FAILED_TARGET_FRIENDLY: return "target is friendly";
            case SPELL_FAILED_TARGET_ENEMY: return "target must be hostile";
            case SPELL_FAILED_NOT_KNOWN: return "spell not learned";
            case SPELL_FAILED_OUT_OF_RANGE: return "out of range";
            case SPELL_FAILED_CASTER_DEAD: return "dead";
            case SPELL_FAILED_CHARMED: return "charmed";
            case SPELL_FAILED_CONFUSED: return "confused";
            case SPELL_FAILED_FLEEING: return "fleeing";
            case SPELL_FAILED_SILENCED: return "silenced";
            case SPELL_FAILED_STUNNED: return "stunned";
            case SPELL_FAILED_SPELL_IN_PROGRESS: return "another action in progress";
            case SPELL_FAILED_ROOTED: return "rooted";
            case SPELL_FAILED_MAINHAND_EMPTY: return "weapon hand empty";
            case SPELL_FAILED_NEED_EXOTIC_AMMO: return "no ammo";
            default: return "code " + std::to_string((int)r);
        }
    }

    std::string Trim(std::string s)
    {
        size_t a = s.find_first_not_of(" \t");
        if (a == std::string::npos)
            return "";
        size_t b = s.find_last_not_of(" \t");
        return s.substr(a, b - a + 1);
    }

    void Tokenize(const std::string& s, std::vector<std::string>& out)
    {
        std::istringstream iss(s);
        std::string tok;
        while (iss >> tok)
            out.push_back(tok);
    }

    void Record(uint32 guidLow, const std::string& cmd, bool ok, const std::string& msg)
    {
        BotCommandAPI::CmdRec rec;
        rec.ts = time(nullptr);
        rec.cmd = cmd;
        rec.ok = ok;
        rec.msg = msg;
        std::deque<BotCommandAPI::CmdRec>& h = g_history[guidLow];
        h.push_back(rec);
        while (h.size() > HISTORY_MAX)
            h.pop_front();
    }
// Cell scan for the nearest lootable corpse with our tap (CellImpl
// idiom; file-scope visitor — local classes can't hold templates).
struct LootScanVisitor
{
    Player* bot;
    Creature* best;
    float bestDist;
    const float RANGE = INTERACTION_DISTANCE + 1.0f;

    void Visit(PlayerMapType&)
    {
    }
    void Visit(CreatureMapType& m)
    {
        for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            Creature* c = itr->getSource();
            if (!c || c->IsAlive() || !c->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE))
                continue;
            if (!c->IsTappedBy(bot))
                continue;
            float d = bot->GetDistance2d(c);
            if (d <= RANGE && d < bestDist)
            {
                best = c;
                bestDist = d;
            }
        }
    }
    template<class T> void Visit(GridRefManager<T>&)
    {
    }
    template<class T> void Visit(T&)
    {
    }
};

}

void BotCommandAPI::Record(uint32 guidLow, const std::string& cmd, bool ok, const std::string& msg)
{
    ::Record(guidLow, cmd, ok, msg);
}

std::string BotCommandAPI::RecentCommandsJson(uint32 guidLow)
{
    std::string out = "[";
    std::map<uint32, std::deque<CmdRec>>::iterator it = g_history.find(guidLow);
    if (it != g_history.end())
    {
        bool first = true;
        for (std::deque<CmdRec>::const_iterator r = it->second.begin(); r != it->second.end(); ++r)
        {
            if (!first)
                out += ",";
            first = false;
            out += std::string("{\"ts\":") + std::to_string((long long)r->ts)
                + ",\"ok\":" + (r->ok ? "true" : "false")
                + ",\"cmd\":\"" + r->cmd + "\""
                + ",\"result\":\"" + r->msg + "\"}";
        }
    }
    out += "]";
    return out;
}

// ---------------------------------------------------------------- movement

std::string BotCommandAPI::CmdMoveTo(Player* bot, const std::vector<std::string>& a)
{
    // a[0]="to"; need x, y [, z]
    if (a.size() < 3)
        return "usage: move to <x> <y> [z]";

    float x = (float)atof(a[1].c_str());
    float y = (float)atof(a[2].c_str());
    float z = a.size() >= 4 ? (float)atof(a[3].c_str()) : bot->GetPositionZ();

    if (std::isnan(x) || std::isnan(y) || std::isnan(z))
        return "bad coordinates";

    // AC BotBuddyAI::MoveTo: clear + direct pathfinding MovePoint
    MotionMaster* mm = bot->GetMotionMaster();
    if (!mm)
        return "no motion master";
    mm->Clear();
    mm->MovePoint(0, x, y, z, MOVE_PATHFINDING);
    return std::string("moving to ") + a[1] + " " + a[2] + " " + a[3];
}

// ------------------------------------------------------------------ combat

std::string BotCommandAPI::CmdAttack(Player* bot, const std::string& guidStr)
{
    uint32 guidLow = (uint32)atol(guidStr.c_str());
    // Low-guid resolution: players are exact (raw low == full guid),
    // creatures/GOs are resolved by a nearby cell scan.
    Unit* target = ResolveUnitByCounter(bot, guidLow, 100.0f);
    if (!target || !bot->IsWithinLOSInMap(target))
        return "target not found or no LOS";

    // AC validation ladder
    if (!bot->IsValidAttackTarget(target))
        return "invalid attack target";
    if (bot->IsFriendlyTo(target))
        return "refusing: friendly target";
    if (Player* tp = target->ToPlayer())
        if (bot->IsInSameGroupWith(tp) || (tp->IsGameMaster() && !bot->IsGameMaster()))
            return "refusing: player target";
    if (!target->IsAlive())
        return "target is dead (use loot)";

    PlayerBotEntry* e = sPlayerBotMgr.GetBot(bot->GetObjectGuid().GetCounter());
    if (e && e->ai)
    {
        e->ai->GetAiObjectContext()->GetValue<Unit*>("current target")->Set(target);
    }

    bot->SetSelectionGuid(target->GetObjectGuid());
    if (bot->IsInCombat() || bot->GetDistance2d(target) <= bot->GetCombatReach() + target->GetCombatReach() + 2.0f)
        bot->SetFacingToObject(target);

    float meleeRange = bot->GetCombatReach() + target->GetCombatReach() + 2.0f;
    float dist = bot->GetDistance2d(target);
    if (dist > meleeRange)
    {
        bot->GetMotionMaster()->MoveChase(target);
        return std::string("attacking ") + (target->GetName() ? target->GetName() : "?") + " (approaching, dist=" + std::to_string((int)dist) + "yd)";
    }
    return std::string("attacking ") + (target->GetName() ? target->GetName() : "?");
}

// ------------------------------------------------------------------- spell

std::string BotCommandAPI::CmdCast(Player* bot, const std::vector<std::string>& a)
{
    if (a.empty())
        return "usage: cast <spellId> [guid]";
    uint32 spellId = (uint32)atol(a[0].c_str());
    SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellId);
    if (!spellInfo)
        return "unknown spell " + a[0];

    Unit* target = nullptr;
    if (a.size() >= 2)
    {
        uint32 guidLow = (uint32)atol(a[1].c_str());
        target = ObjectAccessor::GetUnit(*bot, ObjectGuid(guidLow));
        if (!target)
            return "spell target not found";
        if (target != bot && target->ToPlayer() == nullptr && bot->IsFriendlyTo(target))
            target = bot; // hostile-only spell: self-target fallback
    }
    if (target)
        bot->SetSelectionGuid(target->GetObjectGuid());

    SpellCastResult res = bot->CastSpell(target ? target : bot, spellId, false);
    if (res == SPELL_CAST_OK)
        return "cast " + a[0] + (target && target != bot ? " on " + std::string(target->GetName()) : "");
    return std::string("cast failed: ") + SpellFailReason(res);
}

// -------------------------------------------------------------------- loot

std::string BotCommandAPI::CmdLoot(Player* bot)
{
    Map* map = bot->GetMap();
    if (!map)
        return "no map";

    // Nearest lootable corpse in INTERACTION_DISTANCE
    ObjectGuid lootGuid = bot->GetLootGuid();
    if (!lootGuid.IsEmpty())
    {
        Creature* c = map->GetCreature(lootGuid);
        if (c && c->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE))
        {
            bot->SendLoot(c->GetObjectGuid(), LOOT_CORPSE);
            return "loot opened";
        }
    }

    LootScanVisitor v;
    v.bot = bot;
    v.best = nullptr;
    v.bestDist = v.RANGE + 1.0f;

    CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();
    TypeContainerVisitor<LootScanVisitor, WorldTypeMapContainer> world_vis(v);
    TypeContainerVisitor<LootScanVisitor, GridTypeMapContainer> grid_vis(v);
    cell.Visit(p, world_vis, *map, *bot, v.RANGE);
    cell.Visit(p, grid_vis, *map, *bot, v.RANGE);

    if (!v.best)
        return "no lootable corpse in reach (must have tap)";

    bot->SendLoot(v.best->GetObjectGuid(), LOOT_CORPSE);
    return std::string("loot opened: ") + (v.best->GetName() ? v.best->GetName() : "?");
}

// ---------------------------------------------------------------- interact

std::string BotCommandAPI::CmdInteract(Player* bot, const std::string& guidStr)
{
    uint32 guidLow = (uint32)atol(guidStr.c_str());
    Map* map = bot->GetMap();
    if (!map)
        return "no map";

    // Low guids are NOT full guids in this core (creatures/GOs carry a
    // high part) — resolve by scanning the bot's nearby cells.
    GameObject* go = ResolveGOByCounter(bot, guidLow, INTERACTION_DISTANCE + 1.0f);
    if (go)
    {
        go->Use(bot);
        return "interacted with GO";
    }

    Creature* c = dynamic_cast<Creature*>(ResolveUnitByCounter(bot, guidLow, 40.0f));
    if (c)
    {
        if (!c->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
            return "creature has no interaction";
        bot->PrepareQuestMenu(c->GetObjectGuid());
        // Dump the menu for the agent (questId + icon: 0=new/accept, 1=complete)
        QuestMenu& qm = bot->PlayerTalkClass->GetQuestMenu();
        std::string out = "quest menu (" + std::to_string(qm.MenuItemCount()) + "):";
        for (uint16 i = 0; i < qm.MenuItemCount(); ++i)
        {
            QuestMenuItem const& it = qm.GetItem(i);
            out += " " + std::to_string(it.m_qId) + "(icon" + std::to_string((int)it.m_qIcon) + ")";
        }
        float dist = bot->GetDistance2d(c);
        out += " [giverGuid=" + std::to_string(guidLow) + " dist=" + std::to_string((int)dist) + "yd; accept/turnin need <=6yd]";
        return out;
    }
    return "no such object nearby";
}

// ------------------------------------------------------- accept / turnin

std::string BotCommandAPI::CmdAcceptTurnIn(Player* bot, const std::string& verb, const std::vector<std::string>& a)
{
    // a[0]=questId, a[1]=giverGuid (low guid from the state dump).
    // AC pattern: replay the exact client CMSG through the session handler.
    // NOTE: this core's CMSG_QUESTGIVER_ACCEPT/COMPLETE_QUEST carry
    // (giverGuid, questId) — both required.
    if (a.size() < 2)
        return "usage: " + verb + " <questId> <giverGuid>";
    uint32 questId = (uint32)atol(a[0].c_str());
    uint32 giverLow = (uint32)atol(a[1].c_str());

    WorldSession* session = bot->GetSession();
    if (!session)
        return "no session";

    // (Re)prepare the menu against the giver so the handler's questGiver
    // checks pass, and enforce the server's 5yd interaction range up front.
    Creature* c = dynamic_cast<Creature*>(ResolveUnitByCounter(bot, giverLow, 60.0f));
    if (!c)
        return "giver not found (must be within 60yd)";
    float dist = bot->GetDistance2d(c);
    if (dist > INTERACTION_DISTANCE + 1.0f)
        return "giver out of range (" + std::to_string((int)dist) + "yd, need <=6): move to the giver first";
    bot->PrepareQuestMenu(c->GetObjectGuid());

    WorldPacket pkt;
    pkt.SetOpcode(verb == "accept" ? CMSG_QUESTGIVER_ACCEPT_QUEST : CMSG_QUESTGIVER_COMPLETE_QUEST);
    // The handler resolves the giver via GetObjectByTypeMask, which switches on
    // the guid's HIGH part — the FULL creature guid is required (raw low guid
    // would be mis-read as HIGHGUID_PLAYER).
    pkt << c->GetObjectGuid() << questId;
    if (verb == "accept")
        session->HandleQuestgiverAcceptQuestOpcode(pkt);
    else
        session->HandleQuestgiverCompleteQuest(pkt);
    return verb + " " + std::to_string(questId) + " sent (check quest state)";
}

std::string BotCommandAPI::CmdDrop(Player* bot, const std::string& questIdStr)
{
    uint32 questId = (uint32)atol(questIdStr.c_str());
    WorldSession* session = bot->GetSession();
    if (!session)
        return "no session";
    WorldPacket pkt;
    pkt.SetOpcode(CMSG_QUESTLOG_REMOVE_QUEST);
    pkt << questId;
    session->HandleQuestLogRemoveQuest(pkt);
    return "drop " + std::to_string(questId) + " sent";
}

// -------------------------------------------------------------------- chat

std::string BotCommandAPI::CmdSay(Player* bot, const std::string& text)
{
    if (text.empty())
        return "nothing to say";
    bot->Say(text.c_str(), LANG_UNIVERSAL);
    return "said: " + text;
}

std::string BotCommandAPI::CmdStop(Player* bot)
{
    bot->GetMotionMaster()->Clear();
    PlayerBotEntry* e = sPlayerBotMgr.GetBot(bot->GetObjectGuid().GetCounter());
    if (e && e->ai)
        e->ai->GetAiObjectContext()->GetValue<Unit*>("current target")->Set((Unit*)nullptr);
    bot->SetSelectionGuid(ObjectGuid());
    return "stopped";
}

std::string BotCommandAPI::CmdEngine(Player* bot, const std::string& stateStr)
{
    PlayerBotEntry* e = sPlayerBotMgr.GetBot(bot->GetObjectGuid().GetCounter());
    if (!e || !e->ai)
        return "not a bot";
    int s = atoi(stateStr.c_str());
    if (s < 0 || s >= BOT_STATE_MAX)
        return "usage: engine <0=combat|1=noncombat|2=dead>";
    e->ai->engine->ChangeEngine((BotState)s);
    return std::string("engine set to ") + stateStr;
}

// ------------------------------------------------------------------ dispatch

std::string BotCommandAPI::Execute(Player* bot, const std::string& cmdText)
{
    if (!bot)
        return "no bot";

    std::string cmd = Trim(cmdText);
    if (cmd.empty())
        return "empty command";

    std::vector<std::string> a;
    Tokenize(cmd, a);
    std::string verb = a[0];
    std::transform(verb.begin(), verb.end(), verb.begin(), ::tolower);

    uint32 gl = bot->GetObjectGuid().GetCounter();
    std::string result;

    if (verb == "state")
    {
        result = BotStateSnapshot::BuildBotStateJson(bot);
        BotStateSnapshot::DumpBotFile(bot);
        Record(gl, cmd, true, "snapshot written");
    }
    else if (verb == "move")
    {
        std::vector<std::string> rest(a.begin() + 1, a.end());
        result = CmdMoveTo(bot, rest);
        Record(gl, cmd, result.find("bad") == std::string::npos && result.find("usage") == std::string::npos, result);
    }
    else if (verb == "attack")
    {
        result = a.size() >= 2 ? CmdAttack(bot, a[1]) : "usage: attack <guid>";
        Record(gl, cmd, result.find("not found") == std::string::npos && result.find("refusing") == std::string::npos && result.find("invalid") == std::string::npos && result.find("dead") == std::string::npos && result.find("usage") == std::string::npos, result);
    }
    else if (verb == "cast")
    {
        std::vector<std::string> rest(a.begin() + 1, a.end());
        result = CmdCast(bot, rest);
        Record(gl, cmd, result.find("failed") == std::string::npos && result.find("unknown") == std::string::npos, result);
    }
    else if (verb == "loot")
    {
        result = CmdLoot(bot);
        Record(gl, cmd, result.find("no ") != std::string::npos ? false : true, result);
    }
    else if (verb == "interact")
    {
        result = a.size() >= 2 ? CmdInteract(bot, a[1]) : "usage: interact <guid>";
        Record(gl, cmd, result.find("no such") == std::string::npos && result.find("out of range") == std::string::npos, result);
    }
    else if (verb == "accept" || verb == "turnin")
    {
        std::vector<std::string> rest(a.begin() + 1, a.end());
        result = CmdAcceptTurnIn(bot, verb, rest);
        Record(gl, cmd, result.find("not found") == std::string::npos && result.find("out of range") == std::string::npos && result.find("usage") == std::string::npos, result);
    }
    else if (verb == "drop")
    {
        result = a.size() >= 2 ? CmdDrop(bot, a[1]) : "usage: drop <questId>";
        Record(gl, cmd, true, result);
    }
    else if (verb == "say")
    {
        // Everything after "say "
        size_t pos = cmd.find(' ');
        result = CmdSay(bot, pos != std::string::npos ? Trim(cmd.substr(pos + 1)) : "");
        Record(gl, cmd, !result.empty() && result.find("nothing") == std::string::npos, result);
    }
    else if (verb == "stop")
    {
        result = CmdStop(bot);
        Record(gl, cmd, true, result);
    }
    else if (verb == "engine")
    {
        result = a.size() >= 2 ? CmdEngine(bot, a[1]) : "usage: engine <0|1|2>";
        Record(gl, cmd, result.find("usage") == std::string::npos && result.find("not a bot") == std::string::npos, result);
    }
    else if (verb == "bots")
    {
        result = BotStateSnapshot::BuildBotsListJson();
        BotStateSnapshot::DumpStateFile();
        Record(gl, cmd, true, "bots list written");
    }
    else
    {
        result = "unknown command: " + verb + " (state|move to|attack|cast|loot|interact|accept|turnin|drop|say|stop|engine|bots)";
        Record(gl, cmd, false, result);
    }

    sLog.outInfo("playerbots: agent %s: %s -> %s",
        bot->GetName(), cmd.c_str(),
        result.size() > 200 ? result.substr(0, 200).c_str() : result.c_str());
    return result;
}
