/**
 * @file SelfHealAction.cpp
 * @brief Combat self-heal action implementation (Turtle WoW 1.18.1)
 */
#include "SelfHealAction.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Spells/SpellMgr.h"
#include "DBCStores.h"
#include "AiObjectContext.h"
#include "Logging.h"

namespace
{
// Base (lowest rank) heal spell per class — verified against the online
// bots' character_spell (the IDs the autolearn loop actually grants).
// GetHighestKnownSpell resolves the highest rank the bot has learned.
uint32 GetSelfHealSpell(uint8 cls)
{
    switch (cls)
    {
        case CLASS_PRIEST:  return 2053;   // Lesser Heal
        case CLASS_SHAMAN:  return 332;    // Healing Wave
        case CLASS_PALADIN: return 639;    // Holy Light
        case CLASS_DRUID:   return 5186;   // Healing Touch
        default:            return 0;
    }
}
}

SelfHealAction::SelfHealAction(PlayerBotAI* botAI)
    : Action(botAI, "heal self")
{
}

bool SelfHealAction::isUseful()
{
    if (!bot || !bot->IsAlive())
        return false;

    uint32 base = GetSelfHealSpell(bot->GetClass());
    if (!base)
        return false;

    uint32 spellId = botAI->GetHighestKnownSpell(base);
    if (!spellId || !bot->HasSpell(spellId))
        return false;
    if (bot->HasSpellCooldown(spellId))
        return false;

    SpellEntry const* info = sSpellMgr.GetSpellEntry(spellId);
    if (!info)
        return false;
    Powers powerType = static_cast<Powers>(info->powerType);
    return bot->GetPower(powerType) >= info->manaCost;
}

bool SelfHealAction::Execute([[maybe_unused]] Event event)
{
    if (!bot || !bot->IsAlive())
        return false;

    // "low health" trigger gates this; re-check so we don't heal at full.
    if (bot->GetHealthPercent() > 60)
        return false;

    uint32 base = GetSelfHealSpell(bot->GetClass());
    if (!base)
        return false;

    uint32 spellId = botAI->GetHighestKnownSpell(base);
    if (!spellId || !bot->HasSpell(spellId))
        return false;
    if (bot->HasSpellCooldown(spellId))
        return false;

    SpellEntry const* info = sSpellMgr.GetSpellEntry(spellId);
    if (!info)
        return false;
    Powers powerType = static_cast<Powers>(info->powerType);
    if (bot->GetPower(powerType) < info->manaCost)
        return false;

    bot->CastSpell(bot, spellId, false);
    LOG_DEBUG("playerbots", "%s heal self with %u", bot->GetName(), spellId);
    return true;
}
