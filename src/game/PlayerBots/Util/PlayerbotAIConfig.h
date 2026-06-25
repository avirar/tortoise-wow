#ifndef PLAYERBOT_AI_CONFIG_H
#define PLAYERBOT_AI_CONFIG_H

#include <string>
#include <set>
#include <vector>
#include <mutex>

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

    std::mutex m_logMtx;

private:
    PlayerbotAIConfig();
    ~PlayerbotAIConfig() = default;
    PlayerbotAIConfig(const PlayerbotAIConfig&) = delete;
    PlayerbotAIConfig& operator=(const PlayerbotAIConfig&) = delete;
};

#define sPlayerbotAIConfig PlayerbotAIConfig::instance()

#endif
