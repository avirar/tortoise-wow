#include "AttackAction.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"
#include "AiObjectContext.h"
#include "Value.h"
#include "Timer.h"

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

    bot->SetTargetGuid(target->GetGUID());
    GetAiObjectContext()->GetValue<Unit*>("current target")->Set(target);

    bot->Attack(target, bot->CanReachWithMeleeAutoAttack(target) || true);

    float dist = sServerFacade.GetDistance2d(bot, target);
    if (sServerFacade.IsDistanceGreaterThan(dist, sPlayerbotAIConfig.meleeDistance))
    {
        bot->GetMotionMaster()->MoveChase(target);
    }

    return true;
}

Unit* AttackAction::GetTarget()
{
    Value<Unit*>* targetValue = GetAiObjectContext()->GetValue<Unit*>("current target");
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
    GetAiObjectContext()->GetValue<Unit*>("current target")->Set(nullptr);

    if (bot->IsInCombat())
        bot->CombatStop();

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
