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
    sightDistance = 150.f;
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
    freeMethodLoot = false;

    // Equip upgrade threshold (AC default 1.1 = 10% improvement needed)
    equipUpgradeThreshold = 1.1f;
    xpLevelPenalty = 4.0f;                    // R5c: y/level below bot (XP quality)
    contentionPenalty = 5.0f;                 // R5c: y per bot already on the target
    maxTargetLevelDiff = 4;                   // AC parity: never grind >+4 levels above us
    relocateIdleEnabled = true;               // R5d: self-heal marooned bots
    relocateIdleSeconds = 300;                // R5d/e: idle threshold (>= typical 2-5 min respawn so bots wait out dead spots instead of relocating)

    // R5e: real travel (AC RandomPlayerbotMgr: min/max 1-5h random travel)
    travelEnabled = true;
    travelMinSeconds = 3600;                  // AC: 1h
    travelMaxSeconds = 18000;                 // AC: 5h
    travelCityChancePct = 25;                 // AC probTeleToBankers 0.25
    travelPoiChancePct = 50;                  // R5e POI cache: 50% of local travel → quest POI
    staleCombatSeconds = 600;                 // R5e: 10 min of combat = stuck (typical kills are 30s-3min)
    debugScoreDump = false;                   // R3a P1: one-time item score dump (first few bots)

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
    freeMethodLoot = sConfig.GetBoolDefault("PlayerBot.FreeMethodLoot", freeMethodLoot);
    equipUpgradeThreshold = sConfig.GetFloatDefault("PlayerBot.EquipUpgradeThreshold", equipUpgradeThreshold);
    xpLevelPenalty = sConfig.GetFloatDefault("PlayerBot.XPLevelPenalty", xpLevelPenalty);
    contentionPenalty = sConfig.GetFloatDefault("PlayerBot.ContentionPenalty", contentionPenalty);
    maxTargetLevelDiff = sConfig.GetIntDefault("PlayerBot.MaxTargetLevelDiff", maxTargetLevelDiff);
    relocateIdleEnabled = sConfig.GetBoolDefault("PlayerBot.RelocateIdleEnabled", relocateIdleEnabled);
    relocateIdleSeconds = sConfig.GetIntDefault("PlayerBot.RelocateIdleSeconds", relocateIdleSeconds);
    travelEnabled = sConfig.GetBoolDefault("PlayerBot.TravelEnabled", travelEnabled);
    travelMinSeconds = sConfig.GetIntDefault("PlayerBot.TravelMinSeconds", travelMinSeconds);
    travelMaxSeconds = sConfig.GetIntDefault("PlayerBot.TravelMaxSeconds", travelMaxSeconds);
    travelCityChancePct = sConfig.GetIntDefault("PlayerBot.TravelCityChancePct", travelCityChancePct);
    travelPoiChancePct = sConfig.GetIntDefault("PlayerBot.TravelPoiChancePct", travelPoiChancePct);
    staleCombatSeconds = sConfig.GetIntDefault("PlayerBot.StaleCombatSeconds", staleCombatSeconds);
    debugScoreDump = sConfig.GetBoolDefault("PlayerBot.DebugScoreDump", debugScoreDump);
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
