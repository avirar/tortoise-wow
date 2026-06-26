#include "AttackAction.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"
#include "AiObjectContext.h"
#include "Value.h"
#include "Value/LastMovementValue.h"
#include "Timer.h"
#include "Mgr/Item/LootObjectStack.h"
#include "Strategy/WaitForAttackStrategy.h"
#include "Logging.h"
#include "PlayerbotAIBase.h"

AttackAction::AttackAction(PlayerBotAI* botAI, std::string const& name)
    : MovementAction(botAI, name)
{
}

bool AttackAction::Execute(Event event)
{
    Unit* target = GetTarget();

    if (target)
        return DoAttack(target);

    return false;
}

bool AttackAction::DoAttack(Unit* target)
{
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    if (bot->IsFriendlyTo(target))
        return false;

    if (!bot->IsWithinLOS(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()))
        return false;

    // AC pattern: set selection, save old target, set current target, add to loot stack
    bot->SetTargetGuid(target->GetGUID());
    GetAiObjectContext()->GetValue<Unit*>("old target")->Set(
        GetAiObjectContext()->GetValue<Unit*>("current target")->Get());
    GetAiObjectContext()->GetValue<Unit*>("current target")->Set(target);

    // Add target to loot stack (AC pattern: corpse will be looted when target dies)
    LootObjectStack* lootStack = GetAiObjectContext()->GetValue<LootObjectStack*>("available loot")->Get();
    if (lootStack)
        lootStack->Add(target->GetGUID());

    // AC pattern: clear wander movement if combat priority is higher
    LastMovement& lastMovement = GetAiObjectContext()->GetValue<LastMovement&>("last movement")->Get();
    if (lastMovement.priority < MovementPriority::MOVEMENT_COMBAT && bot->IsMoving())
    {
        lastMovement.clear();
        bot->GetMotionMaster()->Clear(false);
        bot->StopMoving();
    }

    // AC pattern: set facing to target before attacking
    // Always set facing (not gated by CanMove) — bot may be stunned but still needs to face target
    float dist = sServerFacade.GetDistance2d(bot, target);
    float botFacing = bot->GetOrientation();
    float angleToTarget = bot->GetAngle(target);
    bool inArc = bot->HasInArc(target, M_PI_F);
    if (!inArc)
    {
        sServerFacade.SetFacingTo(bot, target);
        LOG_DEBUG("playerbots", "%s [AttackAction] facing fix: bot=%.0f° target=%.0f° dist=%.1f",
            bot->GetName(), botFacing * 57.2958f, angleToTarget * 57.2958f, dist);
    }
    else
    {
        LOG_DEBUG("playerbots", "%s [AttackAction] facing OK: bot=%.0f° target=%.0f° dist=%.1f",
            bot->GetName(), botFacing * 57.2958f, angleToTarget * 57.2958f, dist);
    }

    // AC pattern: set selection so DropTargetAction can clear it
    bot->SetSelectionGuid(target->GetGUID());

    // AC pattern: check WaitForAttack before attacking
    // For solo bots, ShouldWait always returns false (attack immediately)
    if (!WaitForAttackStrategy::ShouldWait(botAI))
        bot->Attack(target, bot->CanReachWithMeleeAutoAttack(target) || true);

    // AC pattern: switch to COMBAT engine when attacking
    botAI->ChangeEngine(BOT_STATE_COMBAT);

    // Move to target if too far for melee
    if (sServerFacade.IsDistanceGreaterThan(dist, sPlayerbotAIConfig.meleeDistance))
    {
        bot->GetMotionMaster()->MoveChase(target);
    }

    LOG_DEBUG("playerbots", "%s [AttackAction] attacking '%s' (entry %u, dist %.1f)",
        bot->GetName(), target->GetName(), target->GetEntry(), dist);

    return true;
}

Unit* AttackAction::GetTarget()
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>(GetTargetName());
    if (targetValue)
    {
        Unit* target = targetValue->Get();
        if (target && target->IsAlive() && target->IsInWorld())
            return target;
    }

    return nullptr;
}

DropTargetAction::DropTargetAction(PlayerBotAI* botAI)
    : Action(botAI, "drop target"), lastDropTime(0)
{
}

bool DropTargetAction::Execute([[maybe_unused]] Event event)
{
    if (!bot || !bot->IsAlive())
        return false;

    // AC pattern: if target is dead, add to loot stack before clearing
    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (target && target->IsDead())
    {
        LootObjectStack* lootStack = GetAiObjectContext()->GetValue<LootObjectStack*>("available loot")->Get();
        if (lootStack)
            lootStack->Add(target->GetGUID());
    }

    // Clear target
    GetAiObjectContext()->GetValue<Unit*>("current target")->Set(nullptr);

    if (bot->GetSelectionGuid().IsEmpty())
        return false;

    bot->SetSelectionGuid(ObjectGuid());

    if (bot->IsInCombat())
        bot->CombatStop();

    bot->AttackStop();

    // AC pattern: switch to NON_COMBAT engine when dropping target
    botAI->ChangeEngine(BOT_STATE_NON_COMBAT);

    lastDropTime = getMSTime();
    return true;
}

bool DropTargetAction::isUseful()
{
    if (getMSTime() - lastDropTime < 1000)
        return false;

    Unit* target = bot->GetVictim();
    if (target && target->IsAlive() && target->IsHostileTo(bot))
        return false;

    return true;
}
