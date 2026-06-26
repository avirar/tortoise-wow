#include "ActionContext.h"

#include "PlayerBotAI.h"
#include "Actions/LootAction.h"
#include "Actions/ReleaseSpiritActions.h"
#include "Actions/ReviveFromCorpseActions.h"
#include "Actions/AcceptResurrectAction.h"

// Class-specific actions (modular registration)
#include "../Class/Warrior/WarriorActions.h"

class ActionContext : public NamedObjectContext<Action>
{
public:
    ActionContext()
    {
        creators["follow"] = &ActionContext::CreateFollowAction;
        creators["flee to group leader"] = &ActionContext::CreateFleeToGroupLeaderAction;
        creators["attack"] = &ActionContext::CreateAttackAction;
        creators["dps assist"] = &ActionContext::CreateDpsAssistAction;
        creators["aggressive target"] = &ActionContext::CreateAggressiveTargetAction;
        creators["drop target"] = &ActionContext::CreateDropTargetAction;
        creators["reach melee"] = &ActionContext::CreateReachMeleeAction;
        creators["reach spell"] = &ActionContext::CreateReachSpellAction;
        creators["flee"] = &ActionContext::CreateFleeAction;
        creators["set facing"] = &ActionContext::CreateSetFacingAction;
        creators["set behind"] = &ActionContext::CreateSetBehindAction;
        creators["enter combat"] = &ActionContext::CreateEnterCombatAction;
        creators["leave combat"] = &ActionContext::CreateLeaveCombatAction;
        creators["move random"] = &ActionContext::CreateMoveRandomAction;
        creators["attack anything"] = &ActionContext::CreateAttackAnythingAction;
        creators["loot"] = &ActionContext::CreateLootAction;
        creators["open loot"] = &ActionContext::CreateOpenLootAction;
        creators["store loot"] = &ActionContext::CreateStoreLootAction;
        creators["equip upgrades"] = &ActionContext::CreateEquipUpgradesAction;
        creators["add all loot"] = &ActionContext::CreateAddAllLootAction;
        creators["move to loot"] = &ActionContext::CreateMoveToLootAction;
        creators["food"] = &ActionContext::CreateEatAction;
        creators["drink"] = &ActionContext::CreateDrinkAction;
        // Death/resurrection actions
        creators["release"] = &ActionContext::CreateReleaseSpiritAction;
        creators["auto release"] = &ActionContext::CreateAutoReleaseSpiritAction;
        creators["repop"] = &ActionContext::CreateRepopAction;
        creators["self resurrect"] = &ActionContext::CreateSelfResurrectAction;
        creators["find corpse"] = &ActionContext::CreateFindCorpseAction;
        creators["revive from corpse"] = &ActionContext::CreateReviveFromCorpseAction;
        creators["spirit healer"] = &ActionContext::CreateSpiritHealerAction;
        creators["accept resurrect"] = &ActionContext::CreateAcceptResurrectAction;

        // === Warrior class actions ===
        creators["battle stance"] = &ActionContext::CreateCastBattleStanceAction;
        creators["defensive stance"] = &ActionContext::CreateCastDefensiveStanceAction;
        creators["berserker stance"] = &ActionContext::CreateCastBerserkerStanceAction;
        creators["battle shout"] = &ActionContext::CreateCastBattleShoutAction;
        creators["bloodrage"] = &ActionContext::CreateCastBloodrageAction;
        creators["berserker rage"] = &ActionContext::CreateCastBerserkerRageAction;
        creators["death wish"] = &ActionContext::CreateCastDeathWishAction;
        creators["recklessness"] = &ActionContext::CreateCastRecklessnessAction;
        creators["shield block"] = &ActionContext::CreateCastShieldBlockAction;
        creators["intimidating shout"] = &ActionContext::CreateCastIntimidatingShoutAction;
        creators["charge"] = &ActionContext::CreateCastChargeAction;
        creators["heroic strike"] = &ActionContext::CreateCastHeroicStrikeAction;
        creators["rend"] = &ActionContext::CreateCastRendAction;
        creators["sunder armor"] = &ActionContext::CreateCastSunderArmorAction;
        creators["thunder clap"] = &ActionContext::CreateCastThunderClapAction;
        creators["hamstring"] = &ActionContext::CreateCastHamstringAction;
        creators["overpower"] = &ActionContext::CreateCastOverpowerAction;
        creators["whirlwind"] = &ActionContext::CreateCastWhirlwindAction;
        creators["mortal strike"] = &ActionContext::CreateCastMortalStrikeAction;
        creators["bloodthirst"] = &ActionContext::CreateCastBloodthirstAction;
        creators["execute"] = &ActionContext::CreateCastExecuteAction;
        creators["shield slam"] = &ActionContext::CreateCastShieldSlamAction;
        creators["revenge"] = &ActionContext::CreateCastRevengeAction;
        creators["taunt"] = &ActionContext::CreateCastTauntAction;
        creators["demoralizing shout"] = &ActionContext::CreateCastDemoralizingShoutAction;
        creators["sweeping strikes"] = &ActionContext::CreateCastSweepingStrikesAction;
    }

private:
    static Action* CreateFollowAction(PlayerBotAI* botAI) { return new FollowAction(botAI); }
    static Action* CreateFleeToGroupLeaderAction(PlayerBotAI* botAI) { return new FleeToGroupLeaderAction(botAI); }
    static Action* CreateAttackAction(PlayerBotAI* botAI) { return new AttackAction(botAI); }
    static Action* CreateDpsAssistAction(PlayerBotAI* botAI) { return new DpsAssistAction(botAI); }
    static Action* CreateAggressiveTargetAction(PlayerBotAI* botAI) { return new AggressiveTargetAction(botAI); }
    static Action* CreateDropTargetAction(PlayerBotAI* botAI) { return new DropTargetAction(botAI); }
    static Action* CreateReachMeleeAction(PlayerBotAI* botAI) { return new ReachCloseCombatAction(botAI); }
    static Action* CreateReachSpellAction(PlayerBotAI* botAI) { return new ReachSpellCombatAction(botAI); }
    static Action* CreateFleeAction(PlayerBotAI* botAI) { return new FleeAction(botAI); }
    static Action* CreateSetFacingAction(PlayerBotAI* botAI) { return new SetFacingAction(botAI); }
    static Action* CreateSetBehindAction(PlayerBotAI* botAI) { return new SetBehindAction(botAI); }
    static Action* CreateEnterCombatAction(PlayerBotAI* botAI) { return new EnterCombatAction(botAI); }
    static Action* CreateLeaveCombatAction(PlayerBotAI* botAI) { return new LeaveCombatAction(botAI); }
    static Action* CreateMoveRandomAction(PlayerBotAI* botAI) { return new MoveRandomAction(botAI); }
    static Action* CreateAttackAnythingAction(PlayerBotAI* botAI) { return new AttackAnythingAction(botAI); }
    static Action* CreateLootAction(PlayerBotAI* botAI) { return new LootAction(botAI); }
    static Action* CreateOpenLootAction(PlayerBotAI* botAI) { return new OpenLootAction(botAI); }
    static Action* CreateStoreLootAction(PlayerBotAI* botAI) { return new StoreLootAction(botAI); }
    static Action* CreateEquipUpgradesAction(PlayerBotAI* botAI) { return new EquipUpgradesAction(botAI); }
    static Action* CreateAddAllLootAction(PlayerBotAI* botAI) { return new AddAllLootAction(botAI); }
    static Action* CreateMoveToLootAction(PlayerBotAI* botAI) { return new MoveToLootAction(botAI); }
    static Action* CreateEatAction(PlayerBotAI* botAI) { return new EatAction(botAI); }
    static Action* CreateDrinkAction(PlayerBotAI* botAI) { return new DrinkAction(botAI); }
    // Death/resurrection actions
    static Action* CreateReleaseSpiritAction(PlayerBotAI* botAI) { return new ReleaseSpiritAction(botAI); }
    static Action* CreateAutoReleaseSpiritAction(PlayerBotAI* botAI) { return new AutoReleaseSpiritAction(botAI); }
    static Action* CreateRepopAction(PlayerBotAI* botAI) { return new RepopAction(botAI); }
    static Action* CreateSelfResurrectAction(PlayerBotAI* botAI) { return new SelfResurrectAction(botAI); }
    static Action* CreateFindCorpseAction(PlayerBotAI* botAI) { return new FindCorpseAction(botAI); }
    static Action* CreateReviveFromCorpseAction(PlayerBotAI* botAI) { return new ReviveFromCorpseAction(botAI); }
    static Action* CreateSpiritHealerAction(PlayerBotAI* botAI) { return new SpiritHealerAction(botAI); }
    static Action* CreateAcceptResurrectAction(PlayerBotAI* botAI) { return new AcceptResurrectAction(botAI); }

    // === Warrior class action factories ===
    static Action* CreateCastBattleStanceAction(PlayerBotAI* botAI) { return new CastBattleStanceAction(botAI); }
    static Action* CreateCastDefensiveStanceAction(PlayerBotAI* botAI) { return new CastDefensiveStanceAction(botAI); }
    static Action* CreateCastBerserkerStanceAction(PlayerBotAI* botAI) { return new CastBerserkerStanceAction(botAI); }
    static Action* CreateCastBattleShoutAction(PlayerBotAI* botAI) { return new CastBattleShoutAction(botAI); }
    static Action* CreateCastBloodrageAction(PlayerBotAI* botAI) { return new CastBloodrageAction(botAI); }
    static Action* CreateCastBerserkerRageAction(PlayerBotAI* botAI) { return new CastBerserkerRageAction(botAI); }
    static Action* CreateCastDeathWishAction(PlayerBotAI* botAI) { return new CastDeathWishAction(botAI); }
    static Action* CreateCastRecklessnessAction(PlayerBotAI* botAI) { return new CastRecklessnessAction(botAI); }
    static Action* CreateCastShieldBlockAction(PlayerBotAI* botAI) { return new CastShieldBlockAction(botAI); }
    static Action* CreateCastIntimidatingShoutAction(PlayerBotAI* botAI) { return new CastIntimidatingShoutAction(botAI); }
    static Action* CreateCastChargeAction(PlayerBotAI* botAI) { return new CastChargeAction(botAI); }
    static Action* CreateCastHeroicStrikeAction(PlayerBotAI* botAI) { return new CastHeroicStrikeAction(botAI); }
    static Action* CreateCastRendAction(PlayerBotAI* botAI) { return new CastRendAction(botAI); }
    static Action* CreateCastSunderArmorAction(PlayerBotAI* botAI) { return new CastSunderArmorAction(botAI); }
    static Action* CreateCastThunderClapAction(PlayerBotAI* botAI) { return new CastThunderClapAction(botAI); }
    static Action* CreateCastHamstringAction(PlayerBotAI* botAI) { return new CastHamstringAction(botAI); }
    static Action* CreateCastOverpowerAction(PlayerBotAI* botAI) { return new CastOverpowerAction(botAI); }
    static Action* CreateCastWhirlwindAction(PlayerBotAI* botAI) { return new CastWhirlwindAction(botAI); }
    static Action* CreateCastMortalStrikeAction(PlayerBotAI* botAI) { return new CastMortalStrikeAction(botAI); }
    static Action* CreateCastBloodthirstAction(PlayerBotAI* botAI) { return new CastBloodthirstAction(botAI); }
    static Action* CreateCastExecuteAction(PlayerBotAI* botAI) { return new CastExecuteAction(botAI); }
    static Action* CreateCastShieldSlamAction(PlayerBotAI* botAI) { return new CastShieldSlamAction(botAI); }
    static Action* CreateCastRevengeAction(PlayerBotAI* botAI) { return new CastRevengeAction(botAI); }
    static Action* CreateCastTauntAction(PlayerBotAI* botAI) { return new CastTauntAction(botAI); }
    static Action* CreateCastDemoralizingShoutAction(PlayerBotAI* botAI) { return new CastDemoralizingShoutAction(botAI); }
    static Action* CreateCastSweepingStrikesAction(PlayerBotAI* botAI) { return new CastSweepingStrikesAction(botAI); }
};

void BuildSharedActionContexts(SharedNamedObjectContextList<Action>& actionContexts)
{
    actionContexts.Add(new ActionContext());
}
