/**
 * @file WarriorActions.cpp
 * @brief Warrior class-specific combat action implementations
 *
 * Uses AI_VALUE2(uint32, "spell id", "spell name") to resolve highest known rank.
 * Spell names match TWoW Spell.dbc Name field exactly.
 */
#include "WarriorActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Spells/SpellMgr.h"
#include "DBCStores.h"
#include "Spells/SpellAuras.h"
#include "Maps/CellImpl.h"
#include "AiObjectContext.h"
#include "Value/Value.h"
#include "Logging.h"
#include "Timer.h"
#include "WarriorAiObjectContext.h"
#include <algorithm>

// ============================================================================
// Helper: cast a spell by name (resolves highest known rank)
// ============================================================================

namespace
{
bool CastWarriorSpell(Player* bot, Unit* target, std::string const& spellName)
{
    if (!bot || !bot->IsAlive())
        return false;

    if (target && (!target->IsAlive() || !target->IsInWorld()))
        return false;

    // Resolve highest known rank by spell name
    uint32 spellId = 0;
    for (PlayerSpellMap::const_iterator itr = bot->GetSpellMap().begin();
         itr != bot->GetSpellMap().end(); ++itr)
    {
        if (itr->second.state == PLAYERSPELL_REMOVED || !itr->second.active)
            continue;

        SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);
        if (!spellInfo || (spellInfo->Attributes & SPELL_ATTR_PASSIVE))
            continue;

        std::string name(spellInfo->SpellName[0]);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::string targetName = spellName;
        std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::tolower);

        if (name == targetName)
        {
            if (!spellId || spellInfo->spellLevel > sSpellMgr.GetSpellEntry(spellId)->spellLevel)
                spellId = itr->first;
        }
    }

    if (!spellId)
    {
        LOG_DEBUG("playerbots", "%s: Spell '%s' not found in spellbook", bot->GetName(), spellName.c_str());
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
    LOG_DEBUG("playerbots", "%s: Cast %s (spellId=%u)", bot->GetName(), spellName.c_str(), spellId);
    return true;
}

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

uint32 GetSunderArmorStacks(Unit* target, Player* bot)
{
    if (!target || !target->IsAlive())
        return 0;

    // Find sunder armor spell ID from bot's spellbook
    uint32 spellId = 0;
    std::string targetName = "sunder armor";
    for (PlayerSpellMap::const_iterator itr = bot->GetSpellMap().begin();
         itr != bot->GetSpellMap().end(); ++itr)
    {
        if (itr->second.state == PLAYERSPELL_REMOVED || !itr->second.active)
            continue;
        SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(itr->first);
        if (!spellInfo) continue;
        std::string name(spellInfo->SpellName[0]);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (name == targetName) { spellId = itr->first; break; }
    }
    if (!spellId) return 0;

    Aura* aura = target->GetAura(spellId, EFFECT_INDEX_0);
    return aura ? aura->GetStackAmount() : 0;
}

} // namespace

// ============================================================================
// Stance Actions
// ============================================================================

CastBattleStanceAction::CastBattleStanceAction(PlayerBotAI* botAI)
    : Action(botAI, "battle stance")
{
}

bool CastBattleStanceAction::Execute([[maybe_unused]] Event event)
{
    return CastWarriorSpell(bot, bot, "battle stance");
}

bool CastBattleStanceAction::isUseful()
{
    // Check if any stance is active
    return !bot->HasAura(2457) && !bot->HasAura(71) && !bot->HasAura(2458);
}

CastDefensiveStanceAction::CastDefensiveStanceAction(PlayerBotAI* botAI)
    : Action(botAI, "defensive stance")
{
}

bool CastDefensiveStanceAction::Execute([[maybe_unused]] Event event)
{
    return CastWarriorSpell(bot, bot, "defensive stance");
}

bool CastDefensiveStanceAction::isUseful()
{
    return !bot->HasAura(71);
}

CastBerserkerStanceAction::CastBerserkerStanceAction(PlayerBotAI* botAI)
    : Action(botAI, "berserker stance")
{
}

bool CastBerserkerStanceAction::Execute([[maybe_unused]] Event event)
{
    return CastWarriorSpell(bot, bot, "berserker stance");
}

bool CastBerserkerStanceAction::isUseful()
{
    return !bot->HasAura(2458);
}

// ============================================================================
// Buff Actions
// ============================================================================

CastBattleShoutAction::CastBattleShoutAction(PlayerBotAI* botAI)
    : Action(botAI, "battle shout")
{
}

bool CastBattleShoutAction::Execute([[maybe_unused]] Event event)
{
    return CastWarriorSpell(bot, bot, "battle shout");
}

bool CastBattleShoutAction::isUseful()
{
    return !TargetHasAuraByName(bot, bot, "battle shout");
}

CastBloodrageAction::CastBloodrageAction(PlayerBotAI* botAI)
    : Action(botAI, "bloodrage")
{
}

bool CastBloodrageAction::Execute([[maybe_unused]] Event event)
{
    return CastWarriorSpell(bot, bot, "bloodrage");
}

bool CastBloodrageAction::isUseful()
{
    return bot->GetPower(POWER_RAGE) < 20 &&
           bot->GetHealthPercent() >= 50;
}

CastBerserkerRageAction::CastBerserkerRageAction(PlayerBotAI* botAI)
    : Action(botAI, "berserker rage")
{
}

bool CastBerserkerRageAction::Execute([[maybe_unused]] Event event)
{
    // Berserker Rage is talent spell, check if known
    uint32 spellId = 1715; // Berserker Rage
    if (!bot->HasSpell(spellId))
        return false;
    return CastWarriorSpell(bot, bot, "berserker rage");
}

bool CastBerserkerRageAction::isUseful()
{
    return bot->HasSpell(1715) &&
           !bot->HasSpellCooldown(1715) &&
           bot->GetPower(POWER_RAGE) < 30;
}

CastDeathWishAction::CastDeathWishAction(PlayerBotAI* botAI)
    : Action(botAI, "death wish")
{
}

bool CastDeathWishAction::Execute([[maybe_unused]] Event event)
{
    return CastWarriorSpell(bot, bot, "death wish");
}

bool CastDeathWishAction::isUseful()
{
    return !bot->HasAura(12328) &&
           !bot->HasSpellCooldown(12328);
}

CastRecklessnessAction::CastRecklessnessAction(PlayerBotAI* botAI)
    : Action(botAI, "recklessness")
{
}

bool CastRecklessnessAction::Execute([[maybe_unused]] Event event)
{
    // Recklessness = 1719 in talent, but it's actually the talent ID not spell
    // In TWoW, Recklessness is spell 1719 (talent)
    return CastWarriorSpell(bot, bot, "recklessness");
}

bool CastRecklessnessAction::isUseful()
{
    // Check if any recklessness-like buff is active
    return !bot->HasAura(1719) &&
           !bot->HasSpellCooldown(1719);
}

CastShieldBlockAction::CastShieldBlockAction(PlayerBotAI* botAI)
    : Action(botAI, "shield block")
{
}

bool CastShieldBlockAction::Execute([[maybe_unused]] Event event)
{
    return CastWarriorSpell(bot, bot, "shield block");
}

bool CastShieldBlockAction::isUseful()
{
    return !bot->HasAura(2565) &&
           !bot->HasSpellCooldown(2565);
}

CastIntimidatingShoutAction::CastIntimidatingShoutAction(PlayerBotAI* botAI)
    : Action(botAI, "intimidating shout")
{
}

bool CastIntimidatingShoutAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetCurrentTarget(botAI);
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "intimidating shout");
}

bool CastIntimidatingShoutAction::isUseful()
{
    Unit* target = GetCurrentTarget(botAI);
    return target && !bot->HasSpellCooldown(12730) &&
           bot->GetDistance(target) <= 8.0f;
}

// ============================================================================
// Melee Offensive Actions
// ============================================================================

// --- Charge ---

CastChargeAction::CastChargeAction(PlayerBotAI* botAI)
    : Action(botAI, "charge")
{
}

Unit* CastChargeAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastChargeAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "charge");
}

bool CastChargeAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    float dist = bot->GetDistance(target);
    return dist > 5.0f && !bot->HasSpellCooldown(100);
}

// --- Heroic Strike ---

CastHeroicStrikeAction::CastHeroicStrikeAction(PlayerBotAI* botAI)
    : Action(botAI, "heroic strike")
{
}

Unit* CastHeroicStrikeAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastHeroicStrikeAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "heroic strike");
}

bool CastHeroicStrikeAction::isUseful()
{
    return bot->GetPower(POWER_RAGE) >= 10;
}

// --- Rend ---

CastRendAction::CastRendAction(PlayerBotAI* botAI)
    : Action(botAI, "rend")
{
}

Unit* CastRendAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastRendAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "rend");
}

bool CastRendAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    return !TargetHasAuraByName(bot, target, "rend") &&
           bot->GetPower(POWER_RAGE) >= 10;
}

// --- Sunder Armor ---

CastSunderArmorAction::CastSunderArmorAction(PlayerBotAI* botAI)
    : Action(botAI, "sunder armor")
{
}

Unit* CastSunderArmorAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastSunderArmorAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "sunder armor");
}

bool CastSunderArmorAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    uint32 stacks = GetSunderArmorStacks(target, bot);
    return stacks < 5 && bot->GetPower(POWER_RAGE) >= 10;
}

// --- Thunder Clap ---

CastThunderClapAction::CastThunderClapAction(PlayerBotAI* botAI)
    : Action(botAI, "thunder clap")
{
}

Unit* CastThunderClapAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastThunderClapAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "thunder clap");
}

bool CastThunderClapAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    if (TargetHasAuraByName(bot, target, "thunder clap"))
        return false;

    if (bot->GetPower(POWER_RAGE) < 10)
        return false;

    // Thunder Clap is 8-yard AOE. Always useful on target in range (debuff + damage).
    // Extra useful when multiple enemies are in AOE radius.
    float aoeRadius = 8.0f;

    // Target must be within AOE radius (buffer to ensure solid hit)
    if (bot->GetDistance(target) > aoeRadius - 2.0f)
        return false;

    return true;
}

// --- Hamstring ---

CastHamstringAction::CastHamstringAction(PlayerBotAI* botAI)
    : Action(botAI, "hamstring")
{
}

Unit* CastHamstringAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastHamstringAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "hamstring");
}

bool CastHamstringAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    return !TargetHasAuraByName(bot, target, "hamstring") &&
           bot->GetPower(POWER_RAGE) >= 10;
}

// --- Overpower ---

CastOverpowerAction::CastOverpowerAction(PlayerBotAI* botAI)
    : Action(botAI, "overpower")
{
}

Unit* CastOverpowerAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastOverpowerAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "overpower");
}

bool CastOverpowerAction::isUseful()
{
    // Overpower only works if target recently dodged/parried/missed
    // Check Unit::m_overpowerReady flag
    Unit* target = GetTarget();
    return target && target->IsAlive() &&
           bot->GetPower(POWER_RAGE) >= 10;
}

// --- Whirlwind ---

CastWhirlwindAction::CastWhirlwindAction(PlayerBotAI* botAI)
    : Action(botAI, "whirlwind")
{
}

Unit* CastWhirlwindAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastWhirlwindAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "whirlwind");
}

bool CastWhirlwindAction::isUseful()
{
    return bot->GetPower(POWER_RAGE) >= 10;
}

// --- Mortal Strike ---

CastMortalStrikeAction::CastMortalStrikeAction(PlayerBotAI* botAI)
    : Action(botAI, "mortal strike")
{
}

Unit* CastMortalStrikeAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastMortalStrikeAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "mortal strike");
}

bool CastMortalStrikeAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    // Check if target already has wounding (mortal strike aura)
    return !TargetHasAuraByName(bot, target, "mortal strike") &&
           bot->GetPower(POWER_RAGE) >= 10;
}

// --- Bloodthirst ---

CastBloodthirstAction::CastBloodthirstAction(PlayerBotAI* botAI)
    : Action(botAI, "bloodthirst")
{
}

Unit* CastBloodthirstAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastBloodthirstAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "bloodthirst");
}

bool CastBloodthirstAction::isUseful()
{
    return bot->GetPower(POWER_RAGE) >= 10;
}

// --- Execute ---

CastExecuteAction::CastExecuteAction(PlayerBotAI* botAI)
    : Action(botAI, "execute")
{
}

Unit* CastExecuteAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastExecuteAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "execute");
}

bool CastExecuteAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    return target->GetHealthPercent() <= 20 &&
           bot->GetPower(POWER_RAGE) >= 10;
}

// --- Shield Slam ---

CastShieldSlamAction::CastShieldSlamAction(PlayerBotAI* botAI)
    : Action(botAI, "shield slam")
{
}

Unit* CastShieldSlamAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastShieldSlamAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "shield slam");
}

bool CastShieldSlamAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    // Shield Slam requires target to have sunder armor or similar armor debuff
    return TargetHasAuraByName(bot, target, "sunder armor") &&
           bot->GetPower(POWER_RAGE) >= 10;
}

// --- Revenge ---

CastRevengeAction::CastRevengeAction(PlayerBotAI* botAI)
    : Action(botAI, "revenge")
{
}

Unit* CastRevengeAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastRevengeAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "revenge");
}

bool CastRevengeAction::isUseful()
{
    // Revenge only works after being blocked
    // Check if bot has "ready for revenge" flag
    return bot->GetPower(POWER_RAGE) >= 10;
}

// --- Taunt ---

CastTauntAction::CastTauntAction(PlayerBotAI* botAI)
    : Action(botAI, "taunt")
{
}

Unit* CastTauntAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastTauntAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "taunt");
}

bool CastTauntAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    Unit* victim = target->GetVictim();
    return victim && victim != bot &&
           !bot->HasSpellCooldown(355);
}

// --- Demoralizing Shout ---

CastDemoralizingShoutAction::CastDemoralizingShoutAction(PlayerBotAI* botAI)
    : Action(botAI, "demoralizing shout")
{
}

Unit* CastDemoralizingShoutAction::GetTarget()
{
    return GetCurrentTarget(botAI);
}

bool CastDemoralizingShoutAction::Execute([[maybe_unused]] Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;
    return CastWarriorSpell(bot, target, "demoralizing shout");
}

bool CastDemoralizingShoutAction::isUseful()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    return !TargetHasAuraByName(bot, target, "demoralizing shout") &&
           bot->GetPower(POWER_RAGE) >= 10;
}

// --- Sweeping Strikes ---

CastSweepingStrikesAction::CastSweepingStrikesAction(PlayerBotAI* botAI)
    : Action(botAI, "sweeping strikes")
{
}

bool CastSweepingStrikesAction::Execute([[maybe_unused]] Event event)
{
    return CastWarriorSpell(bot, bot, "sweeping strikes");
}

bool CastSweepingStrikesAction::isUseful()
{
    return !bot->HasAura(12292) &&
           !bot->HasSpellCooldown(12292);
}
