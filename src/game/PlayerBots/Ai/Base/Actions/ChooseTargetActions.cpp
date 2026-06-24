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

DpsAssistAction::DpsAssistAction(PlayerBotAI* botAI)
    : AttackAction(botAI, "dps assist")
{
}

bool DpsAssistAction::Execute(Event event)
{
    Unit* target = nullptr;

    if (bot->GetVictim())
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
            LOG_DEBUG("playerbots", "%s [DpsAssistAction] path4: SelectNearestTarget=%p",
                      bot->GetName(), (void*)target);
        }
    }

    if (target && target->IsAlive() && !bot->IsFriendlyTo(target))
    {
        LOG_DEBUG("playerbots", "%s [DpsAssistAction] attacking %s", bot->GetName(), target->GetName());
        GetAiObjectContext()->GetValue<Unit*>("current target")->Set(target);
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
    return true;
}

AggressiveTargetAction::AggressiveTargetAction(PlayerBotAI* botAI)
    : AttackAction(botAI, "aggressive target")
{
}

bool AggressiveTargetAction::Execute([[maybe_unused]] Event event)
{
    Unit*       target = bot->SelectNearestTarget(sPlayerbotAIConfig.sightDistance);

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
