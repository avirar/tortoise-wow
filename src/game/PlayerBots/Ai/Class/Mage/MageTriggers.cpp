/**
 * @file MageTriggers.cpp
 * @brief Mage class-specific combat trigger implementations (Turtle WoW 1.18.1)
 */
#include "MageTriggers.h"

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
// Base Polymorph (sheep) — lowest rank.
const uint32 SPELL_POLYMORPH = 118;
}

CanPolymorphTrigger::CanPolymorphTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "can polymorph")
{
}

bool CanPolymorphTrigger::IsActive()
{
    if (!bot || !bot->IsAlive() || !bot->IsInWorld())
        return false;

    // Need the base Polymorph spell
    if (!bot->HasSpell(SPELL_POLYMORPH))
        return false;
    if (bot->HasSpellCooldown(SPELL_POLYMORPH))
        return false;

    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    // Cannot polymorph a higher-level target (below caster level 30)
    if (bot->GetLevel() < 30 && target->GetLevel() > bot->GetLevel())
        return false;

    // Already polymorphed?
    if (target->HasAura(SPELL_POLYMORPH))
        return false;

    return true;
}
