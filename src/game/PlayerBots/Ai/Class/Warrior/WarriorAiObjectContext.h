/**
 * @file WarriorAiObjectContext.h
 * @brief Warrior class-specific context values and registration
 *
 * Uses spell name matching against player's spellbook (version-agnostic).
 */
#ifndef _PLAYERBOT_WARRIOR_AI_OBJECT_CONTEXT_H
#define _PLAYERBOT_WARRIOR_AI_OBJECT_CONTEXT_H

#include "Value/Value.h"
#include "PlayerBotAI.h"
#include "AiObjectContext.h"
#include "Spells/SpellAuras.h"
#include <algorithm>

class PlayerBotAI;

// ============================================================================
// Helper: find spell ID by name from player's spellbook
// ============================================================================

inline uint32 FindWarriorSpellId(Player const* player, std::string const& spellName)
{
    if (!player)
        return 0;

    std::string targetName = spellName;
    std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::tolower);

    uint32 bestId = 0;
    uint8 bestLevel = 0;

    for (PlayerSpellMap::const_iterator itr = player->GetSpellMap().begin();
         itr != player->GetSpellMap().end(); ++itr)
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

// ============================================================================
// Warrior-specific calculated values
// ============================================================================

class WarriorStanceValue : public CalculatedValue<uint32>
{
public:
    WarriorStanceValue(PlayerBotAI* botAI)
        : CalculatedValue<uint32>(botAI, "warrior stance")
    {
    }

    uint32 Calculate() override
    {
        if (bot->HasAura(2457))
            return 0; // Battle Stance
        if (bot->HasAura(71))
            return 1; // Defensive Stance
        if (bot->HasAura(2458))
            return 2; // Berserker Stance (TWoW)
        return 3; // No stance
    }
};

class WarriorRageValue : public CalculatedValue<uint32>
{
public:
    WarriorRageValue(PlayerBotAI* botAI)
        : CalculatedValue<uint32>(botAI, "warrior rage")
    {
    }

    uint32 Calculate() override
    {
        return bot->GetPower(POWER_RAGE);
    }
};

class WarriorHasBattleShoutValue : public CalculatedValue<bool>
{
public:
    WarriorHasBattleShoutValue(PlayerBotAI* botAI)
        : CalculatedValue<bool>(botAI, "warrior has battle shout")
    {
    }

    bool Calculate() override
    {
        uint32 spellId = FindWarriorSpellId(bot, "battle shout");
        return spellId && bot->HasAura(spellId);
    }
};

class WarriorHasRendValue : public CalculatedValue<bool>
{
public:
    WarriorHasRendValue(PlayerBotAI* botAI)
        : CalculatedValue<bool>(botAI, "warrior has rend")
    {
    }

    bool Calculate() override
    {
        Value<Unit*>* targetValue = botAI->GetAiObjectContext()->GetValue<Unit*>("current target");
        if (!targetValue)
            return false;

        Unit* target = targetValue->Get();
        if (!target || !target->IsAlive())
            return false;

        uint32 spellId = FindWarriorSpellId(bot, "rend");
        return spellId && target->HasAura(spellId);
    }
};

class WarriorHasSunderArmorValue : public CalculatedValue<bool>
{
public:
    WarriorHasSunderArmorValue(PlayerBotAI* botAI)
        : CalculatedValue<bool>(botAI, "warrior has sunder armor")
    {
    }

    bool Calculate() override
    {
        Value<Unit*>* targetValue = botAI->GetAiObjectContext()->GetValue<Unit*>("current target");
        if (!targetValue)
            return false;

        Unit* target = targetValue->Get();
        if (!target || !target->IsAlive())
            return false;

        uint32 spellId = FindWarriorSpellId(bot, "sunder armor");
        if (!spellId)
            return false;

        Aura* aura = target->GetAura(spellId, EFFECT_INDEX_0);
        return aura && aura->GetStackAmount() >= 5;
    }
};

class WarriorHasThunderClapValue : public CalculatedValue<bool>
{
public:
    WarriorHasThunderClapValue(PlayerBotAI* botAI)
        : CalculatedValue<bool>(botAI, "warrior has thunder clap")
    {
    }

    bool Calculate() override
    {
        Value<Unit*>* targetValue = botAI->GetAiObjectContext()->GetValue<Unit*>("current target");
        if (!targetValue)
            return false;

        Unit* target = targetValue->Get();
        if (!target || !target->IsAlive())
            return false;

        uint32 spellId = FindWarriorSpellId(bot, "thunder clap");
        return spellId && target->HasAura(spellId);
    }
};

class WarriorTargetHealthPercentValue : public CalculatedValue<float>
{
public:
    WarriorTargetHealthPercentValue(PlayerBotAI* botAI)
        : CalculatedValue<float>(botAI, "warrior target health percent")
    {
    }

    float Calculate() override
    {
        Value<Unit*>* targetValue = botAI->GetAiObjectContext()->GetValue<Unit*>("current target");
        if (!targetValue)
            return 100.0f;

        Unit* target = targetValue->Get();
        if (!target || !target->IsAlive())
            return 100.0f;

        return target->GetHealthPercent();
    }
};

// ============================================================================
// Registration function
// ============================================================================

void BuildWarriorAiObjectContext(PlayerBotAI* botAI);

#endif // _PLAYERBOT_WARRIOR_AI_OBJECT_CONTEXT_H
