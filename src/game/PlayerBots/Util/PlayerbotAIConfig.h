#ifndef PLAYERBOT_AI_CONFIG_H
#define PLAYERBOT_AI_CONFIG_H

#include <string>
#include <set>
#include <vector>
#include <mutex>
#include "PlayerRpgInfo.h"

class PlayerbotAIConfig
{
public:
    static PlayerbotAIConfig& instance()
    {
        static PlayerbotAIConfig instance;
        return instance;
    }

    bool Initialize();

    // Engine
    uint32 iterationsPerTick;
    uint32 expireActionTime;
    uint32 reactDelay;
    uint32 maxWaitForMove;
    bool dynamicReactDelay;

    // R7 L1: movement (AC mod-playerbots MovementAction/NewRpgBaseAction)
    // AC pathFinderDis (NewRpgBaseAction.h:68): straight-walk distance; beyond
    // this MoveFarTo asks the pathfinder for a route.
    float pathFinderDis;
    // AC stuckTime (NewRpgBaseAction.h:76): no 5yd improvement toward the
    // far destination for this long -> stuck-recovery teleport (the only
    // sanctioned teleport in the RPG flow).
    uint32 moveStuckTime;
    // AC AiPlayerbot.MaxMovementSearchTime (default 3): SearchForBestPath
    // z-modification search budget.
    uint32 maxMovementSearchTime;

    // R6.1: agent interface — dump compact bots.json every 30s for the
    // out-of-process agent to poll. Default ON (cheap, ~30KB/30s).
    bool agentStateFile;

    // R6.1: agent interface — file-command queue. Agent drops
    // server/logs/botstate/commands/<bot>.cmd; result in
    // server/logs/botstate/results/<bot>.res (polled every 5s).
    // Bypasses the world console. Default ON.
    bool agentCmdFile;

    // Combat distances
    float sightDistance;
    float spellDistance;
    float reactDistance;
    float meleeDistance;
    float followDistance;
    float aoeRadius;
    float fleeDistance;

    // Health/mana thresholds
    uint32 criticalHealth;
    uint32 lowHealth;
    uint32 mediumHealth;
    uint32 almostFullHealth;
    uint32 lowMana;
    uint32 mediumMana;
    uint32 highMana;
    bool autoSaveMana;
    uint32 saveManaThreshold;

    // Delays
    uint32 globalCoolDown;
    uint32 passiveDelay;
    uint32 repeatDelay;
    uint32 errorDelay;
    uint32 lootDelay;

    // Loot (AC: freeMethodLoot, default false — bots avoid FREE_FOR_ALL loot by default)
    bool freeMethodLoot;

    // Equip upgrade threshold (AC: equipUpgradeThreshold, default 1.1 = 10% improvement)
    float equipUpgradeThreshold;

    // R5c: XP-primary target scoring (replaces the humanoid/gold two-tier).
    // effDist = dist + max(0, botLevel - mobLevel) * xpLevelPenalty
    //               + targetingCount * contentionPenalty
    // Below-level (low-XP) mobs look farther; at/above-level mobs unpenalized
    // (level above bot is still hard-capped by maxTargetLevelDiff). Shyalya/
    // ike3 pattern: soft distance penalties instead of type heuristics.
    float xpLevelPenalty;

    // R5c: effective-distance penalty per bot already targeting the same
    // creature (Shyalya literal value: +5y per targeting player).
    float contentionPenalty;

    // R5: never grind targets more than this many levels above the bot
    // (suicide prevention: city elites like the Stormwind Sewer Beast are
    // otherwise the only "attackable" creature near city gates, causing
    // endless attack->die->corpse-run loops for low-level bots).
    // 4 = AC parity (red mobs +5.. slaughter fresh low-level bots).
    uint32 maxTargetLevelDiff;

    // R5d: self-healing relocation — bots with no viable grind target for
    // this many seconds (alive, overworld, not fighting) teleport back to a
    // level-appropriate band spawn. Fixes graveyard-stacked idlers.
    bool relocateIdleEnabled;
    uint32 relocateIdleSeconds;

    // R5e: real travel (AC RandomPlayerbotMgr pattern) — periodic random
    // travel to real innkeeper/flight/bank hubs, every 1-5h. 25% chance to
    // a city/banker (refreshes the homebind there).
    bool travelEnabled;
    uint32 travelMinSeconds;
    uint32 travelMaxSeconds;
    uint32 travelCityChancePct;
    uint32 travelPoiChancePct;   // non-city travel: quest-POI vs inn/flight/bank hub split
    uint32 staleCombatSeconds;   // R5e: combat longer than this is broken (stuck on unkillable mobs)

    // R7 L2: RPG state machine (AC NewRpgInfo / NewRpgBaseAction::RandomChangeStatus).
    // Per-status auto-selection weight, indexed by RpgStatus (see PlayerRpgInfo.h).
    // 0 = never auto-selected. Defaults: GO_GRIND 50 / WANDER_RANDOM 30 /
    // REST 10 / IDLE 10 / DO_QUEST 0 (deferred to L3+). Sum ~100.
    float rpgStatusProbWeight[RPG_MAX_STATUS];
    // Max time in each status before the state machine returns to IDLE and
    // re-picks (AC NewRpgBaseAction defaults: wander 300s=5min, rest 30s,
    // do-quest 1800s=30min).
    uint32 rpgWanderRandomStatusMaxDuration;
    uint32 rpgRestStatusMaxDuration;
    uint32 rpgDoQuestStatusMaxDuration;
    // Auto-add the "rpg" strategy to the NON_COMBAT engine at login. Default 0
    // for now (existing Wander/Grind/Loot stay the default); flip to 1 once L2
    // is verified, or enable per-bot via the agent interface (`adds rpg`).
    bool rpgEnabled;

    // R7: quest pipeline — town→accept→objective-POI→turn-in loop (AC base
    // quest layer + NewRPG quest state machine, rewritten for our chassis).
    bool questEnabled;
    uint32 questAcceptRadius;      // yd — nearby quest-giver scan range
    uint32 questPoiMaxDist;        // yd — max POI distance (same map + zone)
    uint32 questNoProgressSeconds; // at a POI with zero objective progress → abandon
    uint32 questLogMinFreeSlots;   // below this free-slot count the log is organized

    // R3a P1: one-time item score dump for the first few bots (verifies the
    // StatsWeightCalculator pipeline: base stats / item spells / green suffixes)
    bool debugScoreDump;

    // Random
    uint32 randomChangeMultiplier;

    // Logging
    bool logInGroupOnly;
    bool logValuesPerTick;

    // Combat
    std::string combatStrategies;
    std::string nonCombatStrategies;
    bool fleeingEnabled;

    // Auto-trade
    bool enableAutoTradeOnItemMention;

    // Performance
    bool perfMonEnabled;

    // Persistence (AC: AiPlayerbot.EquipAndSpecPersistence)
    bool persist;

    // AC: AiPlayerbot.DeleteRandomBotAccounts
    // When true, deletes all bot accounts/characters on startup then shuts down
    bool deleteAllBots;

    std::mutex m_logMtx;

private:
    PlayerbotAIConfig();
    ~PlayerbotAIConfig() = default;
    PlayerbotAIConfig(const PlayerbotAIConfig&) = delete;
    PlayerbotAIConfig& operator=(const PlayerbotAIConfig&) = delete;
};

#define sPlayerbotAIConfig PlayerbotAIConfig::instance()

#endif
