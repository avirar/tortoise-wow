#include "DpsTargetValue.h"

#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "PlayerbotAIConfig.h"
#include "Logging.h"

// NOTE: Players CANNOT have threat lists (CanHaveThreatList() returns !IsCreature()).
// Only creatures track threat. For player bots, use GetVictim() and nearby scanning.

DpsTargetValue::DpsTargetValue(PlayerBotAI* botAI)
    : UnitCalculatedValue(botAI, "dps target", 1)
{
}

Unit* DpsTargetValue::Calculate()
{
    Group* group = bot->GetGroup();

    // 1. Bot's current victim (who's actively attacking the bot)
    if (Unit* victim = bot->GetVictim())
    {
        if (victim->IsAlive())
        {
            LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> victim: %s (entry=%u)",
                bot->GetName(), victim->GetName(), victim->GetEntry());
            return victim;
        }
    }

    // 2. Group members' targets (assist pattern)
    // Check who group members are attacking (GetVictim = who THEY attack)
    if (group)
    {
        Unit* bestTarget = nullptr;
        float bestHpPercent = 100.0f;

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member || !member->IsInWorld() || member == bot)
                continue;

            // member->GetVictim() = who the member is attacking
            Unit* memberTarget = member->GetVictim();
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
        {
            LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> group assist: %s (entry=%u)",
                bot->GetName(), bestTarget->GetName(), bestTarget->GetEntry());
            return bestTarget;
        }
    }

    // 3. Master's target (pet/pet-like assist)
    if (Player* master = GetMaster())
    {
        Unit* masterTarget = master->GetVictim();
        if (masterTarget && masterTarget->IsAlive() && !bot->IsFriendlyTo(masterTarget))
        {
            LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> master target: %s (entry=%u)",
                bot->GetName(), masterTarget->GetName(), masterTarget->GetEntry());
            return masterTarget;
        }
    }

    // 4. Fallback: scan nearby unfriendly (neutral + hostile) units
    // Handles: bot in combat but victim=null (attacker stopped swinging),
    // or bot needs proactive target for grinding
    Unit* nearest = bot->SelectNearestUnfriendlyTarget(sPlayerbotAIConfig.sightDistance);
    if (nearest && nearest->IsAlive() && !bot->IsFriendlyTo(nearest))
    {
        LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> nearby: %s (entry=%u)",
            bot->GetName(), nearest->GetName(), nearest->GetEntry());
        return nearest;
    }

    LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> null (victim=%s, inCombat=%s, group=%s)",
        bot->GetName(),
        bot->GetVictim() ? "yes" : "no",
        bot->IsInCombat() ? "yes" : "no",
        group ? "yes" : "no");
    return nullptr;
}
