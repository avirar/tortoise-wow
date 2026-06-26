/**
 * @file WarriorTriggers.cpp
 * @brief Warrior class-specific combat trigger implementations
 *
 * Uses spell name matching against player's spellbook (version-agnostic).
 */
#include "WarriorTriggers.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Spells/SpellMgr.h"
#include "Spells/SpellAuras.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include <algorithm>

// ============================================================================
// Helper: get current target from context
// ============================================================================

namespace
{
Unit* GetCurrentTarget(PlayerBotAI* botAI)
{
    if (!botAI || !botAI->GetAiObjectContext())
        return nullptr;

    Value<Unit*>* targetValue = botAI->GetAiObjectContext()->GetValue<Unit*>("current target");
    if (targetValue)
    {
        Unit* target = targetValue->Get();
        if (target && target->IsAlive() && target->IsInWorld())
            return target;
    }
    return nullptr;
}

// Find a spell ID by name from the player's spellbook
uint32 FindSpellIdByName(Player* bot, std::string const& spellName)
{
    if (!bot)
        return 0;

    std::string targetName = spellName;
    std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::tolower);

    uint32 bestId = 0;
    uint8 bestLevel = 0;

    for (PlayerSpellMap::const_iterator itr = bot->GetSpellMap().begin();
         itr != bot->GetSpellMap().end(); ++itr)
    {
        if (itr->second.state == PLAYERSPELL_REMOVED || !itr->second.active)
            continue;

        SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);
        if (!spellInfo)
            continue;

        std::string name(spellInfo->SpellName[0]);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name == targetName)
        {
            if (!bestId || spellInfo->spellLevel > bestLevel)
            {
                bestId = itr->first;
                bestLevel = spellInfo->spellLevel;
            }
        }
    }
    return bestId;
}

// Check if target has any aura from spells matching the given name
bool TargetHasAuraByName(Player* bot, Unit* target, std::string const& spellName)
{
    if (!target || !target->IsAlive())
        return false;

    std::string targetName = spellName;
    std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::tolower);

    for (PlayerSpellMap::const_iterator itr = bot->GetSpellMap().begin();
         itr != bot->GetSpellMap().end(); ++itr)
    {
        if (itr->second.state == PLAYERSPELL_REMOVED || !itr->second.active)
            continue;

        SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);
        if (!spellInfo)
            continue;

        std::string name(spellInfo->SpellName[0]);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name == targetName && target->HasAura(itr->first))
            return true;
    }
    return false;
}

uint32 GetSunderArmorStacks(Unit* target, Player* bot)
{
    if (!target || !target->IsAlive())
        return 0;

    uint32 spellId = FindSpellIdByName(bot, "sunder armor");
    if (!spellId)
        return 0;

    Aura* aura = target->GetAura(spellId, EFFECT_INDEX_0);
    return aura ? aura->GetStackAmount() : 0;
}

} // namespace

// ============================================================================
// Cooldown triggers
// ============================================================================

CooldownReadyTrigger::CooldownReadyTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "cooldown ready")
{
}

bool CooldownReadyTrigger::IsActive()
{
    return true; // Placeholder - strategies override with specific checks
}

std::string const CooldownReadyTrigger::getName()
{
    return "cooldown ready";
}

// ============================================================================
// Buff triggers
// ============================================================================

BattleShoutExpiredTrigger::BattleShoutExpiredTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "battle shout expired")
{
}

bool BattleShoutExpiredTrigger::IsActive()
{
    uint32 spellId = FindSpellIdByName(bot, "battle shout");
    return !spellId || !bot->HasAura(spellId);
}

SweepingStrikesExpiredTrigger::SweepingStrikesExpiredTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "sweeping strikes expired")
{
}

bool SweepingStrikesExpiredTrigger::IsActive()
{
    // Sweeping Strikes aura
    return !bot->HasAura(12292);
}

// ============================================================================
// Debuff triggers
// ============================================================================

RendExpiredTrigger::RendExpiredTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "rend expired")
{
}

bool RendExpiredTrigger::IsActive()
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;

    return !TargetHasAuraByName(bot, target, "rend");
}

SunderArmorExpiredTrigger::SunderArmorExpiredTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "sunder armor expired")
{
}

bool SunderArmorExpiredTrigger::IsActive()
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;

    uint32 stacks = GetSunderArmorStacks(target, bot);
    return stacks < 5; // Keep 5 stacks
}

ThunderClapExpiredTrigger::ThunderClapExpiredTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "thunder clap expired")
{
}

bool ThunderClapExpiredTrigger::IsActive()
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;

    return !TargetHasAuraByName(bot, target, "thunder clap");
}

// ============================================================================
// Rage triggers
// ============================================================================

LowRageTrigger::LowRageTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "low rage")
{
}

bool LowRageTrigger::IsActive()
{
    return bot->GetPower(POWER_RAGE) < 20;
}

MediumRageTrigger::MediumRageTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "medium rage")
{
}

bool MediumRageTrigger::IsActive()
{
    return bot->GetPower(POWER_RAGE) >= 20 &&
           bot->GetPower(POWER_RAGE) < 50;
}

HighRageTrigger::HighRageTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "high rage")
{
}

bool HighRageTrigger::IsActive()
{
    return bot->GetPower(POWER_RAGE) >= 50;
}

// ============================================================================
// Stance triggers
// ============================================================================

NotInBattleStanceTrigger::NotInBattleStanceTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "not in battle stance")
{
}

bool NotInBattleStanceTrigger::IsActive()
{
    return !bot->HasAura(2457); // Battle Stance
}

NotInDefensiveStanceTrigger::NotInDefensiveStanceTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "not in defensive stance")
{
}

bool NotInDefensiveStanceTrigger::IsActive()
{
    return !bot->HasAura(71); // Defensive Stance
}

NotInBerserkerStanceTrigger::NotInBerserkerStanceTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "not in berserker stance")
{
}

bool NotInBerserkerStanceTrigger::IsActive()
{
    return !bot->HasAura(2458); // Berserker Stance (TWoW ID)
}

// ============================================================================
// Target state triggers
// ============================================================================

TargetBelow20PercentTrigger::TargetBelow20PercentTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "target below 20%")
{
}

bool TargetBelow20PercentTrigger::IsActive()
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;
    return target->GetHealthPercent() <= 20;
}

TargetBelow35PercentTrigger::TargetBelow35PercentTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "target below 35%")
{
}

bool TargetBelow35PercentTrigger::IsActive()
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;
    return target->GetHealthPercent() <= 35;
}

TargetDodgedTrigger::TargetDodgedTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "target dodged")
{
}

bool TargetDodgedTrigger::IsActive()
{
    // Overpower can only be used when target recently dodged/parried/missed
    // Check Unit::m_overpowerReady
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;
    // TODO: check target->m_overpowerReady when available
    return false;
}

// ============================================================================
// Execute ready trigger
// ============================================================================

ExecuteReadyTrigger::ExecuteReadyTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "execute ready")
{
}

bool ExecuteReadyTrigger::IsActive()
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;

    // Execute is usable when target is below 20% HP
    return target->GetHealthPercent() <= 20;
}

// ============================================================================
// Mortal Strike ready trigger
// ============================================================================

MortalStrikeReadyTrigger::MortalStrikeReadyTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "mortal strike ready")
{
}

bool MortalStrikeReadyTrigger::IsActive()
{
    // Mortal Strike needs a target, rage, and no cooldown
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;

    uint32 spellId = FindSpellIdByName(bot, "mortal strike");
    if (!spellId)
        return false;

    // Check cooldown
    if (bot->HasSpellCooldown(spellId))
        return false;

    // Check rage cost (Mortal Strike costs 20 rage)
    if (bot->GetPower(POWER_RAGE) < 20)
        return false;

    return true;
}

// ============================================================================
// Sunder Armor needed trigger
// ============================================================================

SunderArmorNeededTrigger::SunderArmorNeededTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "sunder armor needed")
{
}

bool SunderArmorNeededTrigger::IsActive()
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;

    // Keep 3 stacks for Arms (5 for Protection)
    uint32 stacks = GetSunderArmorStacks(target, bot);
    return stacks < 3;
}

// ============================================================================
// Heroic Strike ready trigger (high rage filler)
// ============================================================================

HeroicStrikeReadyTrigger::HeroicStrikeReadyTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "heroic strike ready")
{
}

bool HeroicStrikeReadyTrigger::IsActive()
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;

    // Heroic Strike needs rage (10 base) and no global cooldown
    if (bot->GetPower(POWER_RAGE) < 10)
        return false;

    // Check if we have the spell (implicit at level 1)
    // Heroic Strike (78) is implicit, always known
    return true;
}

// ============================================================================
// Bloodrage needed trigger (low rage, generate more)
// ============================================================================

BloodrageNeededTrigger::BloodrageNeededTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "bloodrage needed")
{
}

bool BloodrageNeededTrigger::IsActive()
{
    // Bloodrage when rage is low and not already active
    if (bot->GetPower(POWER_RAGE) >= 30)
        return false;

    uint32 spellId = FindSpellIdByName(bot, "bloodrage");
    if (!spellId)
        return false;

    // Check cooldown
    if (bot->HasSpellCooldown(spellId))
        return false;

    return true;
}

// ============================================================================
// Special triggers
// ============================================================================

DeathWishReadyTrigger::DeathWishReadyTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "death wish ready")
{
}

bool DeathWishReadyTrigger::IsActive()
{
    return !bot->HasAura(12328) &&
           !bot->HasSpellCooldown(12328);
}

ShieldBlockReadyTrigger::ShieldBlockReadyTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "shield block ready")
{
}

bool ShieldBlockReadyTrigger::IsActive()
{
    return !bot->HasAura(2565) &&
           !bot->HasSpellCooldown(2565);
}

IntimidatingShoutReadyTrigger::IntimidatingShoutReadyTrigger(PlayerBotAI* botAI)
    : Trigger(botAI, "intimidating shout ready")
{
}

bool IntimidatingShoutReadyTrigger::IsActive()
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;

    return !bot->HasSpellCooldown(12730) &&
           bot->GetDistance(target) <= 8.0f;
}
