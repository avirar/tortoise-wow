#include "TriggerContext.h"

#include "PlayerBotAI.h"
#include "Trigger/CombatTrigger.h"
#include "Trigger/LootTriggers.h"
#include "Trigger/DeathTriggers.h"

// Class-specific triggers (modular registration)
#include "../Class/Warrior/WarriorTriggers.h"

class TriggerContext : public NamedObjectContext<Trigger>
{
public:
    TriggerContext()
    {
        creators["enemy out of melee"] = &TriggerContext::CreateEnemyOutOfMelee;
        creators["enemy out of spell"] = &TriggerContext::CreateEnemyOutOfSpell;
        creators["enemy too close for spell"] = &TriggerContext::CreateEnemyTooCloseForSpell;
        creators["invalid target"] = &TriggerContext::CreateInvalidTarget;
        creators["not facing target"] = &TriggerContext::CreateNotFacingTarget;
        creators["not behind target"] = &TriggerContext::CreateNotBehindTarget;
        creators["low health"] = &TriggerContext::CreateLowHealth;
        creators["medium health"] = &TriggerContext::CreateMediumHealth;
        creators["low mana"] = &TriggerContext::CreateLowMana;
        creators["high mana"] = &TriggerContext::CreateHighMana;
        creators["often"] = &TriggerContext::CreateOften;
        creators["no target"] = &TriggerContext::CreateNoTarget;
        creators["not dps target active"] = &TriggerContext::CreateNotDpsTargetActive;
        creators["has target"] = &TriggerContext::CreateHasTarget;
        creators["loot available"] = &TriggerContext::CreateLootAvailable;
        creators["far from loot target"] = &TriggerContext::CreateFarFromLoot;
        creators["can loot"] = &TriggerContext::CreateCanLoot;
        creators["loot open"] = &TriggerContext::CreateLootOpen;
        // Death/resurrection triggers
        creators["dead"] = &TriggerContext::CreateDead;
        creators["corpse near"] = &TriggerContext::CreateCorpseNear;
        creators["resurrect request"] = &TriggerContext::CreateResurrectRequest;
        creators["can self resurrect"] = &TriggerContext::CreateCanSelfResurrect;
        creators["falling far"] = &TriggerContext::CreateFallingFar;

        // === Warrior class triggers ===
        creators["battle shout expired"] = &TriggerContext::CreateBattleShoutExpired;
        creators["rend expired"] = &TriggerContext::CreateRendExpired;
        creators["sunder armor expired"] = &TriggerContext::CreateSunderArmorExpired;
        creators["thunder clap expired"] = &TriggerContext::CreateThunderClapExpired;
        creators["low rage"] = &TriggerContext::CreateLowRage;
        creators["medium rage"] = &TriggerContext::CreateMediumRage;
        creators["high rage"] = &TriggerContext::CreateHighRage;
        creators["not in battle stance"] = &TriggerContext::CreateNotInBattleStance;
        creators["not in defensive stance"] = &TriggerContext::CreateNotInDefensiveStance;
        creators["not in berserker stance"] = &TriggerContext::CreateNotInBerserkerStance;
        creators["target below 20%"] = &TriggerContext::CreateTargetBelow20Percent;
        creators["target below 35%"] = &TriggerContext::CreateTargetBelow35Percent;
        creators["target dodged"] = &TriggerContext::CreateTargetDodged;
        creators["death wish ready"] = &TriggerContext::CreateDeathWishReady;
        creators["shield block ready"] = &TriggerContext::CreateShieldBlockReady;
        creators["intimidating shout ready"] = &TriggerContext::CreateIntimidatingShoutReady;
        creators["execute ready"] = &TriggerContext::CreateExecuteReady;
        creators["mortal strike ready"] = &TriggerContext::CreateMortalStrikeReady;
        creators["sunder armor needed"] = &TriggerContext::CreateSunderArmorNeeded;
        creators["heroic strike ready"] = &TriggerContext::CreateHeroicStrikeReady;
        creators["bloodrage needed"] = &TriggerContext::CreateBloodrageNeeded;
    }

private:
    static Trigger* CreateEnemyOutOfMelee(PlayerBotAI* botAI) { return new EnemyOutOfMeleeTrigger(botAI); }
    static Trigger* CreateEnemyOutOfSpell(PlayerBotAI* botAI) { return new EnemyOutOfSpellTrigger(botAI); }
    static Trigger* CreateEnemyTooCloseForSpell(PlayerBotAI* botAI) { return new EnemyTooCloseForSpellTrigger(botAI); }
    static Trigger* CreateInvalidTarget(PlayerBotAI* botAI) { return new InvalidTargetTrigger(botAI); }
    static Trigger* CreateNotFacingTarget(PlayerBotAI* botAI) { return new NotFacingTargetTrigger(botAI); }
    static Trigger* CreateNotBehindTarget(PlayerBotAI* botAI) { return new NotBehindTargetTrigger(botAI); }
    static Trigger* CreateLowHealth(PlayerBotAI* botAI) { return new LowHealthTrigger(botAI); }
    static Trigger* CreateMediumHealth(PlayerBotAI* botAI) { return new MediumHealthTrigger(botAI); }
    static Trigger* CreateLowMana(PlayerBotAI* botAI) { return new LowManaTrigger(botAI); }
    static Trigger* CreateHighMana(PlayerBotAI* botAI) { return new HighManaTrigger(botAI); }
    static Trigger* CreateOften(PlayerBotAI* botAI) { return new RandomTrigger(botAI, "often", 7); }
    static Trigger* CreateNoTarget(PlayerBotAI* botAI) { return new NoTargetTrigger(botAI); }
    static Trigger* CreateNotDpsTargetActive(PlayerBotAI* botAI) { return new NotDpsTargetActiveTrigger(botAI); }
    static Trigger* CreateHasTarget(PlayerBotAI* botAI) { return new HasTargetTrigger(botAI); }
    static Trigger* CreateLootAvailable(PlayerBotAI* botAI) { return new LootAvailableTrigger(botAI); }
    static Trigger* CreateFarFromLoot(PlayerBotAI* botAI) { return new FarFromLootTrigger(botAI); }
    static Trigger* CreateCanLoot(PlayerBotAI* botAI) { return new CanLootTrigger(botAI); }
    static Trigger* CreateLootOpen(PlayerBotAI* botAI) { return new LootOpenTrigger(botAI); }
    // Death/resurrection triggers
    static Trigger* CreateDead(PlayerBotAI* botAI) { return new DeadTrigger(botAI); }
    static Trigger* CreateCorpseNear(PlayerBotAI* botAI) { return new CorpseNearTrigger(botAI); }
    static Trigger* CreateResurrectRequest(PlayerBotAI* botAI) { return new ResurrectRequestTrigger(botAI); }
    static Trigger* CreateCanSelfResurrect(PlayerBotAI* botAI) { return new CanSelfResurrectTrigger(botAI); }
    static Trigger* CreateFallingFar(PlayerBotAI* botAI) { return new FallingFarTrigger(botAI); }

    // === Warrior class trigger factories ===
    static Trigger* CreateBattleShoutExpired(PlayerBotAI* botAI) { return new BattleShoutExpiredTrigger(botAI); }
    static Trigger* CreateRendExpired(PlayerBotAI* botAI) { return new RendExpiredTrigger(botAI); }
    static Trigger* CreateSunderArmorExpired(PlayerBotAI* botAI) { return new SunderArmorExpiredTrigger(botAI); }
    static Trigger* CreateThunderClapExpired(PlayerBotAI* botAI) { return new ThunderClapExpiredTrigger(botAI); }
    static Trigger* CreateLowRage(PlayerBotAI* botAI) { return new LowRageTrigger(botAI); }
    static Trigger* CreateMediumRage(PlayerBotAI* botAI) { return new MediumRageTrigger(botAI); }
    static Trigger* CreateHighRage(PlayerBotAI* botAI) { return new HighRageTrigger(botAI); }
    static Trigger* CreateNotInBattleStance(PlayerBotAI* botAI) { return new NotInBattleStanceTrigger(botAI); }
    static Trigger* CreateNotInDefensiveStance(PlayerBotAI* botAI) { return new NotInDefensiveStanceTrigger(botAI); }
    static Trigger* CreateNotInBerserkerStance(PlayerBotAI* botAI) { return new NotInBerserkerStanceTrigger(botAI); }
    static Trigger* CreateTargetBelow20Percent(PlayerBotAI* botAI) { return new TargetBelow20PercentTrigger(botAI); }
    static Trigger* CreateTargetBelow35Percent(PlayerBotAI* botAI) { return new TargetBelow35PercentTrigger(botAI); }
    static Trigger* CreateTargetDodged(PlayerBotAI* botAI) { return new TargetDodgedTrigger(botAI); }
    static Trigger* CreateDeathWishReady(PlayerBotAI* botAI) { return new DeathWishReadyTrigger(botAI); }
    static Trigger* CreateShieldBlockReady(PlayerBotAI* botAI) { return new ShieldBlockReadyTrigger(botAI); }
    static Trigger* CreateIntimidatingShoutReady(PlayerBotAI* botAI) { return new IntimidatingShoutReadyTrigger(botAI); }
    static Trigger* CreateExecuteReady(PlayerBotAI* botAI) { return new ExecuteReadyTrigger(botAI); }
    static Trigger* CreateMortalStrikeReady(PlayerBotAI* botAI) { return new MortalStrikeReadyTrigger(botAI); }
    static Trigger* CreateSunderArmorNeeded(PlayerBotAI* botAI) { return new SunderArmorNeededTrigger(botAI); }
    static Trigger* CreateHeroicStrikeReady(PlayerBotAI* botAI) { return new HeroicStrikeReadyTrigger(botAI); }
    static Trigger* CreateBloodrageNeeded(PlayerBotAI* botAI) { return new BloodrageNeededTrigger(botAI); }
};

void BuildSharedTriggerContexts(SharedNamedObjectContextList<Trigger>& triggerContexts)
{
    triggerContexts.Add(new TriggerContext());
}
