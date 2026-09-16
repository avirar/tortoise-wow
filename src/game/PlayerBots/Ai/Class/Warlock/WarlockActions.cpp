/**
 * @file WarlockActions.cpp
 * @brief Warlock class-specific combat action implementations (Turtle WoW 1.18.1)
 */
#include "WarlockActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Spells/SpellMgr.h"
#include "DBCStores.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "Logging.h"

namespace
{
const uint32 SPELL_FEAR = 5782;

// Helper: cast a spell by base ID on the target, resolving highest known
// rank, checking cooldown, mana, and range.
bool CastWarlockSpell(PlayerBotAI* ai, Player* bot, Unit* target, uint32 baseSpellId)
{
    if (!bot || !bot->IsAlive())
        return false;
    if (target && (!target->IsAlive() || !target->IsInWorld()))
        return false;

    uint32 spellId = ai->GetHighestKnownSpell(baseSpellId);
    if (!spellId)
        return false;
    if (bot->HasSpellCooldown(spellId))
        return false;

    SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellId);
    if (!spellInfo)
        return false;

    Powers powerType = static_cast<Powers>(spellInfo->powerType);
    if (bot->GetPower(powerType) < (uint32)spellInfo->manaCost)
        return false;

    SpellRangeEntry const* rangeEntry = sSpellRangeStore.LookupEntry(spellInfo->rangeIndex);
    float spellRange = rangeEntry ? rangeEntry->maxRange : 0.0f;
    if (target && spellRange > 0.0f && bot->GetDistance(target) > spellRange)
        return false;

    bot->CastSpell(target ? target : bot, spellId, false);
    LOG_DEBUG("playerbots", "%s cast %u (target %s)", bot->GetName(), spellId,
              target ? target->GetName() : "self");
    return true;
}
}

CastFearAction::CastFearAction(PlayerBotAI* botAI)
    : Action(botAI, "fear")
{
}

bool CastFearAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    return CastWarlockSpell(botAI, bot, target, SPELL_FEAR);
}

bool CastFearAction::isUseful()
{
    return true; // Gated by the "can fear" trigger
}
