#include "DpsTargetValue.h"

#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "PlayerbotAIConfig.h"
#include "Logging.h"

// AC pattern: DpsTargetValue returns enemies actively attacking the bot or group members.
// Used by "dps assist" for DEFENSE (reactive), not grinding (proactive).
// Grinding is handled by GrindTargetValue -> "attack anything".
//
// Tortoise note: players cannot have threat lists (CanHaveThreatList() returns false).
// Use Unit::GetAttackers() (m_attackers set) instead - populated by Unit::Attack()/AttackStop().

DpsTargetValue::DpsTargetValue(PlayerBotAI* botAI)
    : UnitCalculatedValue(botAI, "dps target", 1)
{
}

Unit* DpsTargetValue::Calculate()
{
    Group* group = bot->GetGroup();
    Unit* victim = bot->GetVictim();
    size_t attackerCount = bot->GetAttackers().size();

    LOG_DEBUG("playerbots", "%s [DpsTargetValue] START: group=%s, inCombat=%s, victim=%s, attackers=%zu",
        bot->GetName(),
        group ? "yes" : "no",
        bot->IsInCombat() ? "yes" : "no",
        victim ? victim->GetName() : "null",
        attackerCount);

    // 1. Bot's current victim (who bot is actively attacking)
    if (victim && victim->IsAlive())
    {
        LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> victim: %s (entry=%u)",
            bot->GetName(), victim->GetName(), victim->GetEntry());
        return victim;
    }

    // 2. Bot's attackers (units actively attacking the bot, even if bot hasn't attacked back)
    // This is the self-defense case: bear attacked Samanna, she needs to fight back
    Unit::AttackerSet const& myAttackers = bot->GetAttackers();
    for (Unit* attacker : myAttackers)
    {
        if (!attacker || !attacker->IsAlive() || bot->IsFriendlyTo(attacker))
            continue;
        LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> my attacker: %s (entry=%u)",
            bot->GetName(), attacker->GetName(), attacker->GetEntry());
        return attacker;
    }

    // 3. Group members' targets and attackers (assist pattern)
    // Find the lowest-HP enemy threatening any group member
    if (group)
    {
        Unit* bestTarget = nullptr;
        float bestHpPercent = 100.0f;

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member || !member->IsInWorld() || member == bot)
                continue;

            // Member's victim (who member is attacking)
            if (Unit* memberTarget = member->GetVictim())
            {
                if (memberTarget->IsAlive() && !bot->IsFriendlyTo(memberTarget))
                {
                    float hpPercent = memberTarget->GetHealthPercent();
                    LOG_DEBUG("playerbots", "%s [DpsTargetValue] group member %s victim: %s (entry=%u, hp=%.0f%%)",
                        bot->GetName(), member->GetName(), memberTarget->GetName(),
                        memberTarget->GetEntry(), hpPercent);
                    if (hpPercent < bestHpPercent)
                    {
                        bestHpPercent = hpPercent;
                        bestTarget = memberTarget;
                    }
                }
            }

            // Member's attackers (who is attacking this member)
            for (Unit* attacker : member->GetAttackers())
            {
                if (!attacker || !attacker->IsAlive() || bot->IsFriendlyTo(attacker))
                    continue;

                float hpPercent = attacker->GetHealthPercent();
                LOG_DEBUG("playerbots", "%s [DpsTargetValue] group member %s attacker: %s (entry=%u, hp=%.0f%%)",
                    bot->GetName(), member->GetName(), attacker->GetName(),
                    attacker->GetEntry(), hpPercent);
                if (hpPercent < bestHpPercent)
                {
                    bestHpPercent = hpPercent;
                    bestTarget = attacker;
                }
            }
        }

        if (bestTarget)
        {
            LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> group best: %s (entry=%u)",
                bot->GetName(), bestTarget->GetName(), bestTarget->GetEntry());
            return bestTarget;
        }
    }

    // 4. Master's target (pet-like assist)
    if (Player* master = GetMaster())
    {
        if (Unit* masterTarget = master->GetVictim())
        {
            if (masterTarget->IsAlive() && !bot->IsFriendlyTo(masterTarget))
            {
                LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> master victim: %s (entry=%u)",
                    bot->GetName(), masterTarget->GetName(), masterTarget->GetEntry());
                return masterTarget;
            }
        }

        for (Unit* attacker : master->GetAttackers())
        {
            if (attacker && attacker->IsAlive() && !bot->IsFriendlyTo(attacker))
            {
                LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> master attacker: %s (entry=%u)",
                    bot->GetName(), attacker->GetName(), attacker->GetEntry());
                return attacker;
            }
        }
    }

    LOG_DEBUG("playerbots", "%s [DpsTargetValue] -> null (no active attackers)",
        bot->GetName());
    return nullptr;
}
