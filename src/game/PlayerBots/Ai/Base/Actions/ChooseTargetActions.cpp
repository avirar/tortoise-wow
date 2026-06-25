#include "ChooseTargetActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "Group.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"
#include "AiObjectContext.h"
#include "Value.h"
#include "ObjectAccessor.h"
#include "Logging.h"
#include "Log.h"
#include "Timer.h"
#include <map>

DpsAssistAction::DpsAssistAction(PlayerBotAI* botAI)
    : AttackAction(botAI, "dps assist")
{
}

bool DpsAssistAction::Execute(Event event)
{
    Unit* target = nullptr;

    if (bot->GetVictim() && bot->GetVictim()->IsAlive() && bot->GetVictim()->IsInWorld())
    {
        target = bot->GetVictim();
        LOG_DEBUG("playerbots", "%s [DpsAssistAction] path1: own victim %s", bot->GetName(), target->GetName());
    }
    else
    {
        Group* group = bot->GetGroup();
        if (group)
        {
            float lowestHpPercent = 1.0f;
            uint32 memberCount = 0;
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->getSource();
                if (!member || !member->IsInWorld() || member == bot)
                    continue;
                memberCount++;

                ObjectGuid memberTargetGuid = member->GetSelectionGuid();
                Unit* memberTarget = memberTargetGuid.IsCreature() ? ObjectAccessor::GetUnit(*bot, memberTargetGuid) : nullptr;
                LOG_DEBUG("playerbots", "%s [DpsAssistAction] group member %s, selection=%u, inCombat=%d",
                          bot->GetName(), member->GetName(), memberTargetGuid.GetCounter(), member->IsInCombat() ? 1 : 0);

                if (memberTarget && memberTarget->IsAlive() && member->IsInCombat())
                {
                    LOG_DEBUG("playerbots", "%s [DpsAssistAction]   -> assist target %s",
                              bot->GetName(), memberTarget->GetName());
                    float hpPercent = (float)memberTarget->GetHealth() / (float)memberTarget->GetMaxHealth();
                    if (hpPercent < lowestHpPercent)
                    {
                        lowestHpPercent = hpPercent;
                        target = memberTarget;
                    }
                }
            }
            LOG_DEBUG("playerbots", "%s [DpsAssistAction] group iteration done, checked %u members, target=%p",
                      bot->GetName(), memberCount, (void*)target);
        }
        else
        {
            LOG_DEBUG("playerbots", "%s [DpsAssistAction] no group", bot->GetName());
        }

        if (!target)
        {
            Player* master = GetMaster();
            if (master)
            {
                ObjectGuid masterTargetGuid = master->GetSelectionGuid();
                Unit* masterTarget = masterTargetGuid.IsCreature() ? ObjectAccessor::GetUnit(*bot, masterTargetGuid) : nullptr;
                LOG_DEBUG("playerbots", "%s [DpsAssistAction] path3: master %s, selection=%u, resolved=%p",
                          bot->GetName(), master->GetName(), masterTargetGuid.GetCounter(), (void*)masterTarget);
                if (masterTarget)
                {
                    target = masterTarget;
                }
            }
            else
            {
                LOG_DEBUG("playerbots", "%s [DpsAssistAction] path3: no master", bot->GetName());
            }
        }

        if (!target)
        {
            target = bot->SelectNearestTarget(sPlayerbotAIConfig.sightDistance);
            LOG_DEBUG("playerbots", "%s [DpsAssistAction] path4: SelectNearestTarget=%p pos=(%.1f,%.1f)",
                      bot->GetName(), (void*)target, bot->GetPositionX(), bot->GetPositionY());
            if (!target)
            {
                // Also try unfriendly (neutral) search
                target = sServerFacade.SelectNearestHostileTarget(bot, sPlayerbotAIConfig.sightDistance);
                LOG_DEBUG("playerbots", "%s [DpsAssistAction] path4b: SelectNearestHostileTarget=%p (entry=%u '%s')",
                    bot->GetName(), (void*)target,
                    target ? target->GetEntry() : 0u,
                    target ? target->GetName() : "(nil)");
            }
        }
    }

    if (target && target->IsAlive() && !bot->IsFriendlyTo(target))
    {
        GetAiObjectContext()->GetValue<Unit*>("current target")->Set(target);
        // DoAttack() handles both in-range (attack) and out-of-range (chase)
        return DoAttack(target);
    }

    LOG_DEBUG("playerbots", "%s [DpsAssistAction] no valid target, final target=%p alive=%d friendly=%d",
              bot->GetName(),
              (void*)target,
              target && target->IsAlive() ? 1 : 0,
              target && bot->IsFriendlyTo(target) ? 1 : 0);

    return false;
}

bool DpsAssistAction::isUseful()
{
    // AC: always useful, Execute() handles target selection
    return true;
}

AttackAnythingAction::AttackAnythingAction(PlayerBotAI* botAI)
    : AttackAction(botAI, "attack anything")
{
}

bool AttackAnythingAction::Execute(Event /*event*/)
{
    Unit* target = sServerFacade.SelectNearestSafeTarget(bot, sPlayerbotAIConfig.sightDistance);

    if (!target || !target->IsAlive() || bot->IsFriendlyTo(target))
    {
        LOG_DEBUG("playerbots", "%s [AttackAnything::Execute] SelectNearestSafeTarget returned nil, pos=(%.1f,%.1f)",
            bot->GetName(), bot->GetPositionX(), bot->GetPositionY());
        return false;
    }

    // Double-check tap at execution time (race condition: tap set between isUseful and Execute)
    if (Creature* c = target->ToCreature())
    {
        if (c->HasLootRecipient() && !c->IsTappedBy(bot))
            return false;  // tapped by someone else, skip
    }

    LOG_DEBUG("playerbots", "%s [AttackAnything::Execute] found target entry=%u '%s' dist=%.1f reaction=%d",
        bot->GetName(), target->GetEntry(), target->GetName(),
        sServerFacade.GetDistance2d(bot, target), (int)bot->GetReactionTo(target));

    // Skip training dummies
    std::string name = target->GetName();
    if (name.find("Dummy") != std::string::npos ||
        name.find("Charge Target") != std::string::npos ||
        name.find("Melee Target") != std::string::npos ||
        name.find("Ranged Target") != std::string::npos)
        return false;

    GetAiObjectContext()->GetValue<Unit*>("current target")->Set(target);
    return DoAttack(target);
}

bool AttackAnythingAction::isUseful()
{
    // Don't block on IsInCombat() — bot may just have dropped its target
    // and is waiting for server to clear combat flag. Engine state controls
    // whether "attack anything" should be active (via trigger nodes).
    Unit* target = sServerFacade.SelectNearestSafeTarget(bot, sPlayerbotAIConfig.sightDistance);
    if (!target || !target->IsAlive() || bot->IsFriendlyTo(target))
    {
        // Debug: scan nearby creatures every 30s when no target found
        static std::map<std::string, uint32> lastDebugTime;
        uint32 now = getMSTime();
        if (lastDebugTime.find(bot->GetName()) == lastDebugTime.end() || now - lastDebugTime[bot->GetName()] > 30000)
        {
            lastDebugTime[bot->GetName()] = now;
            sServerFacade.DebugNearbyCreatures(bot, sPlayerbotAIConfig.sightDistance, "AttackAnything::isUseful");
        }
        return false;
    }

    return true;
}

bool AttackAnythingAction::isPossible()
{
    Unit* target = sServerFacade.SelectNearestSafeTarget(bot, sPlayerbotAIConfig.sightDistance);
    return target && AttackAction::isPossible();
}

AggressiveTargetAction::AggressiveTargetAction(PlayerBotAI* botAI)
    : AttackAction(botAI, "aggressive target")
{
}

bool AggressiveTargetAction::Execute([[maybe_unused]] Event event)
{
    Unit*       target = sServerFacade.SelectNearestHostileTarget(bot, sPlayerbotAIConfig.sightDistance);

    if (target && target->IsAlive() && target->IsHostileTo(bot))
    {
        GetAiObjectContext()->GetValue<Unit*>("current target")->Set(target);
        return DoAttack(target);
    }

    return false;
}

bool AggressiveTargetAction::isUseful()
{
    return !bot->IsInCombat();
}
