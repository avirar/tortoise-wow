/**
 * @file PaladinTriggers.cpp
 * @brief Paladin class-specific combat trigger implementations (Turtle WoW 1.18.1)
 */
#include "PaladinTriggers.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Spells/SpellMgr.h"
#include "Spells/SpellAuras.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "Logging.h"

namespace
{
// Base Hammer of Justice — lowest rank.
const uint32 SPELL_HAMMER_OF_JUSTICE = 853;
}

CanHammerOfJusticeTrigger::CanHammerOfJusticeTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "can hammer of justice")
{
}

bool CanHammerOfJusticeTrigger::IsActive()
{
    if (!bot || !bot->IsAlive() || !bot->IsInWorld())
        return false;

    // Need HoJ (any rank) and it off cooldown
    uint32 spellId = botAI->GetHighestKnownSpell(SPELL_HAMMER_OF_JUSTICE);
    if (!spellId || !bot->HasSpell(spellId))
        return false;
    if (bot->HasSpellCooldown(spellId))
        return false;

    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    // HoJ is a short-range stun; require melee reach
    if (!bot->CanReachWithMeleeAutoAttack(target))
        return false;

    // Already stunned?
    if (target->HasAuraType(SPELL_AURA_MOD_STUN))
        return false;

    return true;
}
