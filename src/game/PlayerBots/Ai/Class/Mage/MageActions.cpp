/**
 * @file MageActions.cpp
 * @brief Mage class-specific combat action implementations (Turtle WoW 1.18.1)
 */
#include "MageActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Spells/SpellMgr.h"
#include "DBCStores.h"
#include "Spells/SpellAuras.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "Logging.h"
#include "Timer.h"
#include <algorithm>

namespace
{
// Base spell IDs (lowest rank) for range/cost checks.
const uint32 SPELL_POLYMORPH = 118;

// Helper: cast a spell by base ID, resolving highest known rank via spell
// chain, checking cooldowns, mana cost, and range.
bool CastMageSpell(PlayerBotAI* ai, Player* bot, Unit* target, uint32 baseSpellId)
{
    if (!bot || !bot->IsAlive())
        return false;

    if (target && (!target->IsAlive() || !target->IsInWorld()))
        return false;

    // Resolve highest known rank from the base spell's chain
    uint32 spellId = ai->GetHighestKnownSpell(baseSpellId);
    if (!spellId)
    {
        LOG_DEBUG("playerbots", "%s: spell %u not known", bot->GetName(), baseSpellId);
        return false;
    }

    if (bot->HasSpellCooldown(spellId))
        return false;

    SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spellId);
    if (!spellInfo)
        return false;

    // Check power cost
    Powers powerType = static_cast<Powers>(spellInfo->powerType);
    if (bot->GetPower(powerType) < (uint32)spellInfo->manaCost)
        return false;

    // Check range
    SpellRangeEntry const* rangeEntry = sSpellRangeStore.LookupEntry(spellInfo->rangeIndex);
    float spellRange = rangeEntry ? rangeEntry->maxRange : 0.0f;
    if (target && spellRange > 0.0f)
    {
        float dist = bot->GetDistance(target);
        if (dist > spellRange)
            return false;
    }

    bot->CastSpell(target ? target : bot, spellId, false);
    LOG_DEBUG("playerbots", "%s cast %u (target %s)", bot->GetName(), spellId,
              target ? target->GetName() : "self");
    return true;
}
}

// ============================================================================
// PolymorphAction
// ============================================================================

CastPolymorphAction::CastPolymorphAction(PlayerBotAI* botAI)
    : Action(botAI, "polymorph")
{
}

bool CastPolymorphAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    return CastMageSpell(botAI, bot, target, SPELL_POLYMORPH);
}

bool CastPolymorphAction::isUseful()
{
    return true; // Trigger (can polymorph) gates this
}
