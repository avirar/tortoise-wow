#include "CombatTrigger.h"

#include "Player.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "PlayerbotAIConfig.h"
#include "Timer.h"
#include "AiObjectContext.h"
#include "Value/Value.h"

EnemyOutOfMeleeTrigger::EnemyOutOfMeleeTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "enemy out of melee")
{
}

bool EnemyOutOfMeleeTrigger::IsActive()
{
    Unit* target = bot->GetVictim();
    if (!target)
        target = ObjectAccessor::GetUnit(*bot, bot->GetSelectionGuid());

    if (!target)
        return false;

    return !bot->CanReachWithMeleeAutoAttack(target);
}

EnemyOutOfSpellTrigger::EnemyOutOfSpellTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "enemy out of spell")
{
}

bool EnemyOutOfSpellTrigger::IsActive()
{
    Unit* target = bot->GetVictim();
    if (!target)
        target = ObjectAccessor::GetUnit(*bot, bot->GetSelectionGuid());

    if (!target)
        return false;

    float dist = bot->GetDistance(target);
    return dist > sPlayerbotAIConfig.spellDistance;
}

EnemyTooCloseForSpellTrigger::EnemyTooCloseForSpellTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "enemy too close for spell")
{
}

bool EnemyTooCloseForSpellTrigger::IsActive()
{
    Unit* target = bot->GetVictim();
    if (!target)
        target = ObjectAccessor::GetUnit(*bot, bot->GetSelectionGuid());

    if (!target)
        return false;

    float dist = bot->GetDistance(target);
    return dist < sPlayerbotAIConfig.meleeDistance;
}

InvalidTargetTrigger::InvalidTargetTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "invalid target")
{
}

bool InvalidTargetTrigger::IsActive()
{
    Unit* target = bot->GetVictim();
    if (!target)
        target = ObjectAccessor::GetUnit(*bot, bot->GetSelectionGuid());

    if (!target)
        return true;

    if (!target->IsAlive())
        return true;

    if (!target->IsInWorld())
        return true;

    if (bot->IsFriendlyTo(target))
        return true;

    return false;
}

NotFacingTargetTrigger::NotFacingTargetTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "not facing target")
{
}

bool NotFacingTargetTrigger::IsActive()
{
    Unit* target = bot->GetVictim();
    if (!target)
        target = ObjectAccessor::GetUnit(*bot, bot->GetSelectionGuid());

    if (!target)
        return false;

    return !bot->HasInArc(target, M_PI_F);
}

NotBehindTargetTrigger::NotBehindTargetTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "not behind target")
{
}

bool NotBehindTargetTrigger::IsActive()
{
    Unit* target = bot->GetVictim();
    if (!target)
        target = ObjectAccessor::GetUnit(*bot, bot->GetSelectionGuid());

    if (!target)
        return false;

    return target->HasInArc(bot, M_PI_F);
}

RandomTrigger::RandomTrigger(PlayerBotAI* botAI, std::string const& name, int32 probability)
    : Trigger(botAI, name), probability(probability), lastCheck(0)
{
}

bool RandomTrigger::IsActive()
{
    uint32 now = getMSTime();
    if (now - lastCheck < sPlayerbotAIConfig.repeatDelay)
        return false;

    lastCheck = now;

    int32 k = probability / sPlayerbotAIConfig.randomChangeMultiplier;
    if (k < 1)
        k = 1;

    return (rand() % k) == 0;
}

NoTargetTrigger::NoTargetTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "no target")
{
}

bool NoTargetTrigger::IsActive()
{
    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    return !target;
}
