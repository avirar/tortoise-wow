#include "PlayerbotAIConfig.h"
#include "Config/Config.h"

PlayerbotAIConfig::PlayerbotAIConfig()
{
    // Engine
    iterationsPerTick = 5;
    expireActionTime = 1000;
    reactDelay = 100;
    maxWaitForMove = 10000;
    dynamicReactDelay = false;

    // Combat distances
    sightDistance = 50.f;
    spellDistance = 30.f;
    reactDistance = 40.f;
    meleeDistance = 5.f;
    followDistance = 15.f;
    aoeRadius = 8.f;
    fleeDistance = 40.f;

    // Health/mana thresholds
    criticalHealth = 15;
    lowHealth = 30;
    mediumHealth = 60;
    almostFullHealth = 90;
    lowMana = 10;
    mediumMana = 30;
    highMana = 60;
    autoSaveMana = true;
    saveManaThreshold = 20;

    // Delays
    globalCoolDown = 1500;
    passiveDelay = 1000;
    repeatDelay = 500;
    errorDelay = 5000;
    lootDelay = 1000;

    // Random
    randomChangeMultiplier = 1;

    // Logging
    logInGroupOnly = true;
    logValuesPerTick = false;

    // Combat
    combatStrategies = "default melee;default ranged";
    nonCombatStrategies = "follow;loot;stay";
    fleeingEnabled = true;

    // Auto-trade
    enableAutoTradeOnItemMention = false;

    // Performance
    perfMonEnabled = false;

    // Persistence
    persist = true;

    // Delete all bots (AC: AiPlayerbot.DeleteRandomBotAccounts)
    deleteAllBots = false;
}

bool PlayerbotAIConfig::Initialize()
{
    iterationsPerTick = sConfig.GetIntDefault("PlayerBot.IterationsPerTick", iterationsPerTick);
    expireActionTime = sConfig.GetIntDefault("PlayerBot.ExpireActionTime", expireActionTime);
    reactDelay = sConfig.GetIntDefault("PlayerBot.ReactDelay", reactDelay);
    maxWaitForMove = sConfig.GetIntDefault("PlayerBot.MaxWaitForMove", maxWaitForMove);
    dynamicReactDelay = sConfig.GetBoolDefault("PlayerBot.DynamicReactDelay", dynamicReactDelay);

    sightDistance = (float)sConfig.GetIntDefault("PlayerBot.SightDistance", (int)sightDistance);
    spellDistance = (float)sConfig.GetIntDefault("PlayerBot.SpellDistance", (int)spellDistance);
    reactDistance = (float)sConfig.GetIntDefault("PlayerBot.ReactDistance", (int)reactDistance);
    meleeDistance = (float)sConfig.GetIntDefault("PlayerBot.MeleeDistance", (int)meleeDistance);
    followDistance = (float)sConfig.GetIntDefault("PlayerBot.FollowDistance", (int)followDistance);
    aoeRadius = (float)sConfig.GetIntDefault("PlayerBot.AoeRadius", (int)aoeRadius);
    fleeDistance = (float)sConfig.GetIntDefault("PlayerBot.FleeDistance", (int)fleeDistance);

    criticalHealth = sConfig.GetIntDefault("PlayerBot.CriticalHealth", criticalHealth);
    lowHealth = sConfig.GetIntDefault("PlayerBot.LowHealth", lowHealth);
    mediumHealth = sConfig.GetIntDefault("PlayerBot.MediumHealth", mediumHealth);
    almostFullHealth = sConfig.GetIntDefault("PlayerBot.AlmostFullHealth", almostFullHealth);
    lowMana = sConfig.GetIntDefault("PlayerBot.LowMana", lowMana);
    mediumMana = sConfig.GetIntDefault("PlayerBot.MediumMana", mediumMana);
    highMana = sConfig.GetIntDefault("PlayerBot.HighMana", highMana);
    autoSaveMana = sConfig.GetBoolDefault("PlayerBot.AutoSaveMana", autoSaveMana);
    saveManaThreshold = sConfig.GetIntDefault("PlayerBot.SaveManaThreshold", saveManaThreshold);

    globalCoolDown = sConfig.GetIntDefault("PlayerBot.GlobalCooldown", globalCoolDown);
    passiveDelay = sConfig.GetIntDefault("PlayerBot.PassiveDelay", passiveDelay);
    repeatDelay = sConfig.GetIntDefault("PlayerBot.RepeatDelay", repeatDelay);
    errorDelay = sConfig.GetIntDefault("PlayerBot.ErrorDelay", errorDelay);
    lootDelay = sConfig.GetIntDefault("PlayerBot.LootDelay", lootDelay);
    randomChangeMultiplier = sConfig.GetIntDefault("PlayerBot.RandomChangeMultiplier", randomChangeMultiplier);

    logInGroupOnly = sConfig.GetBoolDefault("PlayerBot.LogInGroupOnly", logInGroupOnly);
    logValuesPerTick = sConfig.GetBoolDefault("PlayerBot.LogValuesPerTick", logValuesPerTick);

    fleeingEnabled = sConfig.GetBoolDefault("PlayerBot.FleeingEnabled", fleeingEnabled);
    enableAutoTradeOnItemMention = sConfig.GetBoolDefault("PlayerBot.EnableAutoTradeOnItemMention", enableAutoTradeOnItemMention);
    perfMonEnabled = sConfig.GetBoolDefault("PlayerBot.PerfMonEnabled", perfMonEnabled);

    // Persistence
    persist = sConfig.GetBoolDefault("PlayerBot.Persist", persist);

    // Delete all bots (AC: AiPlayerbot.DeleteRandomBotAccounts)
    deleteAllBots = sConfig.GetBoolDefault("PlayerBot.DeleteAllBots", deleteAllBots);

    return true;
}
