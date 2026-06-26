#include "DpsTargetValue.h"

#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "PlayerbotAIConfig.h"
#include "Logging.h"

DpsTargetValue::DpsTargetValue(PlayerBotAI* botAI)
    : UnitCalculatedValue(botAI, "dps target", 1)
{
}

Unit* DpsTargetValue::Calculate()
{
    // If bot has a victim (being attacked by or attacking), keep it
    if (bot->GetVictim() && bot->GetVictim()->IsAlive())
    {
        LOG_DEBUG("playerbots", "%s [DpsTargetValue] victim: %s (entry=%u)",
            bot->GetName(), bot->GetVictim()->GetName(), bot->GetVictim()->GetEntry());
        return bot->GetVictim();
    }

    Group* group = bot->GetGroup();
    if (group)
    {
        Unit* bestTarget = nullptr;
        float bestHpPercent = 100.0f;

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member || !member->IsInWorld() || member == bot)
                continue;

            Unit* memberTarget = member->GetVictim();
            if (!memberTarget)
                memberTarget = ObjectAccessor::GetUnit(*member, member->GetSelectionGuid());

            if (memberTarget && memberTarget->IsAlive() && !bot->IsFriendlyTo(memberTarget))
            {
                float hpPercent = memberTarget->GetHealthPercent();
                if (hpPercent < bestHpPercent)
                {
                    bestHpPercent = hpPercent;
                    bestTarget = memberTarget;
                }
            }
        }

        if (bestTarget)
            return bestTarget;
    }

    if (Player* master = GetMaster())
    {
        Unit* masterTarget = master->GetVictim();
        if (!masterTarget)
            masterTarget = ObjectAccessor::GetUnit(*master, master->GetSelectionGuid());

        if (masterTarget && masterTarget->IsAlive() && !bot->IsFriendlyTo(masterTarget))
            return masterTarget;
    }

    // Solo fallback: scan nearby unfriendly (neutral + hostile) units
    Unit* nearest = bot->SelectNearestUnfriendlyTarget(sPlayerbotAIConfig.sightDistance);
    if (nearest && nearest->IsAlive() && !bot->IsFriendlyTo(nearest))
        return nearest;

    return nullptr;
}
