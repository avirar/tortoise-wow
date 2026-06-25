#include "ActionContext.h"

#include "PlayerBotAI.h"
#include "Actions/LootAction.h"

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
};

void BuildSharedActionContexts(SharedNamedObjectContextList<Action>& actionContexts)
{
    actionContexts.Add(new ActionContext());
}
