/*
 *  BotCommandAPI — the R6.1 agent control surface.
 *  One free-text command in, one result line out. Commands (v1):
 *    state                        full JSON snapshot (+ file dump)
 *    move to <x> <y> [z]          walk (pathfinding), AC BotBuddyAI::MoveTo semantics
 *    attack <guid>                AC BotBuddyAI::Attack validation + selection
 *    cast <spellId> [guid]        spell by id (target via selection)
 *    loot                         open nearest lootable corpse in reach
 *    interact <guid>              GO use / quest-giver menu dump
 *    accept <questId> [giverGuid] CMSG replay (session handler, AC pattern)
 *    turnin <questId> [giverGuid] CMSG replay
 *    drop <questId>               questlog remove (CMSG replay)
 *    say <text>                   chat
 *    stop                         clear movement + target
 *    engine <0|1|2>               force engine state (debug)
 *    bots                         compact all-bots JSON
 *
 *  Port context: AC mod-ollama-bot-buddy's BotBuddyAI (api.cpp) executes
 *  commands from the in-process Ollama loop; we expose the same semantics
 *  through the world console (R6.1) for the out-of-process agent.
 */
#ifndef PLAYERBOT_BOTCOMMAND_API_H
#define PLAYERBOT_BOTCOMMAND_API_H

#include <string>
#include <vector>
#include <time.h>

class Player;

class BotCommandAPI
{
public:
    struct CmdRec
    {
        time_t ts;
        std::string cmd;
        bool ok;
        std::string msg;
    };

    // Execute one command; returns the result line (never throws).
    static std::string Execute(Player* bot, const std::string& cmdText);

    // JSON of the last N agent commands for this bot (snapshot "commands").
    static std::string RecentCommandsJson(uint32 guidLow);

private:
    static void Record(uint32 guidLow, const std::string& cmd, bool ok, const std::string& msg);

    static std::string CmdMoveTo(Player* bot, const std::vector<std::string>& a);
    static std::string CmdAttack(Player* bot, const std::string& guidStr);
    static std::string CmdCast(Player* bot, const std::vector<std::string>& a);
    static std::string CmdLoot(Player* bot);
    static std::string CmdInteract(Player* bot, const std::string& guidStr);
    static std::string CmdAcceptTurnIn(Player* bot, const std::string& verb, const std::vector<std::string>& a);
    static std::string CmdDrop(Player* bot, const std::string& questIdStr);
    static std::string CmdSay(Player* bot, const std::string& text);
    static std::string CmdStop(Player* bot);
    static std::string CmdEngine(Player* bot, const std::string& stateStr);
};

#endif
