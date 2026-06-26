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
        // AC pattern: only consider targets of group members actively in combat
        // Use GetVictim() (active combat target), NOT GetSelectionGuid() (just selected)
        Unit* bestTarget = nullptr;
        float bestHpPercent = 100.0f;

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member || !member->IsInWorld() || member == bot)
                continue;

            // Only use targets from members actively in combat
            if (!member->IsInCombat())
                continue;

            Unit* memberTarget = member->GetVictim();
            if (!memberTarget)
                continue;  // in combat but no victim (shouldn't happen, skip)

            if (memberTarget->IsAlive() && !bot->IsFriendlyTo(memberTarget))
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

    // Master check: only if master is actively in combat with a victim
    if (Player* master = GetMaster())
    {
        if (!master->IsInCombat())
            return nullptr;  // master not in combat, don't assist

        Unit* masterTarget = master->GetVictim();
        if (masterTarget && masterTarget->IsAlive() && !bot->IsFriendlyTo(masterTarget))
            return masterTarget;
    }

    // AC pattern: when in a group, only assist group members' targets
    // Don't fall back to solo target selection (bot should follow/assist, not grind)
    if (group)
        return nullptr;

    // Solo fallback: scan nearby unfriendly (neutral + hostile) units
    Unit* nearest = bot->SelectNearestUnfriendlyTarget(sPlayerbotAIConfig.sightDistance);
    if (nearest && nearest->IsAlive() && !bot->IsFriendlyTo(nearest))
        return nearest;

    return nullptr;
}
