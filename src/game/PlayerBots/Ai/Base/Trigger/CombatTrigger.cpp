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
    // Check "current target" context value (AC pattern: AI_VALUE2(bool, "invalid target", "current target"))
    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (!target)
        return true;

    // AC InvalidTargetValue::Calculate() checks: dead, different map, not visible, friendly, etc.
    // Tortoise adaptation: IsVisible() → IsInWorld() (tortoise lacks IsVisible())
    return target->GetMapId() != bot->GetMapId() ||
           !target->IsInWorld() ||
           !target->IsAlive() ||
           target->IsFriendlyTo(bot);
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
    // AC pattern: only checks if "current target" is null
    // Dead/stale targets are handled by InvalidTargetTrigger → DropTargetAction
    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    return !target;
}
