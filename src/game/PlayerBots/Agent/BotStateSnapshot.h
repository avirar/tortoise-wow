/*
 *  BotStateSnapshot — agent observability contract (R6.1).
 *  JSON snapshots of bot state for the out-of-process agent (LLM) side:
 *    - BuildBotStateJson(bot): full per-bot state (identity/location/
 *      resources/combat/nearby/inventory/quests/recent commands)
 *    - BuildBotOneLineJson(bot): compact row for the bots list
 *    - BuildBotsListJson(): all online bots, compact rows
 *    - DumpStateFile(): full compact list -> server/logs/botstate/bots.json
 *    - DumpBotFile(bot): full snapshot -> server/logs/botstate/<name>.json
 *
 *  Port context (R6): AC mod-ollama-bot-buddy built a free-text prompt
 *  per bot (loop.cpp); we emit machine-readable JSON instead — the agent
 *  lives outside the process and reads the state file / console output.
 */
#ifndef PLAYERBOT_BOTSTATE_SNAPSHOT_H
#define PLAYERBOT_BOTSTATE_SNAPSHOT_H

class Player;

class BotStateSnapshot
{
public:
    static std::string BuildBotStateJson(Player* bot);
    static std::string BuildBotOneLineJson(Player* bot);
    static std::string BuildBotsListJson();
    static void DumpStateFile();
    static void DumpBotFile(Player* bot);
};

#endif
