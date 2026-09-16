/**
 * @file WarlockTriggers.cpp
 * @brief Warlock class-specific combat trigger implementations (Turtle WoW 1.18.1)
 */
#include "WarlockTriggers.h"

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
// Base Fear — lowest rank. Bots have it (verified in character_spell).
const uint32 SPELL_FEAR = 5782;
}

CanFearTrigger::CanFearTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "can fear")
{
}

bool CanFearTrigger::IsActive()
{
    if (!bot || !bot->IsAlive() || !bot->IsInWorld())
        return false;

    // Need Fear (any rank) and it off cooldown
    uint32 spellId = botAI->GetHighestKnownSpell(SPELL_FEAR);
    if (!spellId || !bot->HasSpell(spellId))
        return false;
    if (bot->HasSpellCooldown(spellId))
        return false;

    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    // Already feared?
    if (target->IsFeared())
        return false;

    return true;
}
