#include "TriggerContext.h"

#include "PlayerBotAI.h"
#include "Trigger/LootTriggers.h"

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
        creators["loot available"] = &TriggerContext::CreateLootAvailable;
        creators["far from loot target"] = &TriggerContext::CreateFarFromLoot;
        creators["can loot"] = &TriggerContext::CreateCanLoot;
        creators["loot open"] = &TriggerContext::CreateLootOpen;
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
    static Trigger* CreateLootAvailable(PlayerBotAI* botAI) { return new LootAvailableTrigger(botAI); }
    static Trigger* CreateFarFromLoot(PlayerBotAI* botAI) { return new FarFromLootTrigger(botAI); }
    static Trigger* CreateCanLoot(PlayerBotAI* botAI) { return new CanLootTrigger(botAI); }
    static Trigger* CreateLootOpen(PlayerBotAI* botAI) { return new LootOpenTrigger(botAI); }
};

void BuildSharedTriggerContexts(SharedNamedObjectContextList<Trigger>& triggerContexts)
{
    triggerContexts.Add(new TriggerContext());
}
