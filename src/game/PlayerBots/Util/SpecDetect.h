/*
 * The Grand Protocol v3/gpl-3.0 License
 * tortoise-wow PlayerBot engine port — spec detection (P1-3)
 *
 * Header-only vanilla talent-tab detection used for class-strategy
 * selection (and stats weighting). Lifted from StatsWeightCalculator.cpp
 * statics so both consumers share one implementation.
 *
 * Vanilla talent tab layout per class (TalentTab 0/1/2):
 * Warrior: 0=Arms, 1=Fury, 2=Protection
 * Paladin: 0=Holy, 1=Protection, 2=Retribution
 * Hunter: 0=Beast Mastery, 1=Marksmanship, 2=Survival
 * Rogue: 0=Assassination, 1=Combat, 2=Subtlety
 * Priest: 0=Holy, 1=Shadow, 2=Discipline
 * Shaman: 0=Elemental, 1=Enhancement, 2=Restoration
 * Mage: 0=Arcane, 1=Fire, 2=Frost
 * Warlock: 0=Affliction, 1=Demonology, 2=Destruction
 * Druid: 0=Feral Combat, 1=Restoration, 2=Balance
 */
#ifndef PLAYERBOT_SPECDTECT_H
#define PLAYERBOT_SPECDTECT_H

#include <map>
#include "Player.h"
#include "DBCStores.h"

namespace PlayerbotSpec
{
    // Talent points invested per tree, read from the player spell map
    inline std::map<uint8, uint32> GetTalentPointsPerTab(Player* player)
    {
        std::map<uint8, uint32> tabs = {{0, 0}, {1, 0}, {2, 0}};

        PlayerSpellMap const& spellMap = player->GetSpellMap();
        for (PlayerSpellMap::const_iterator itr = spellMap.begin(); itr != spellMap.end(); ++itr)
        {
            uint32 spellId = itr->first;
            TalentSpellPos const* talentPos = GetTalentSpellPos(spellId);
            if (!talentPos)
                continue;

            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentPos->talent_id);
            if (!talentInfo)
                continue;

            uint8 tab = static_cast<uint8>(talentInfo->TalentTab);
            tabs[tab] += talentPos->rank;
        }

        return tabs;
    }

    // Dominant tree (most points invested). Below level 10 (no talent
    // points possible before 10) fall back to sensible class defaults.
    inline uint8 DetectSpecTab(Player* player)
    {
        if (player->GetLevel() < 10)
        {
            switch (player->GetClass())
            {
                case CLASS_WARRIOR:   return 0; // Arms
                case CLASS_PALADIN:   return 2; // Retribution
                case CLASS_HUNTER:    return 1; // Marksmanship
                case CLASS_ROGUE:     return 1; // Combat
                case CLASS_PRIEST:    return 0; // Holy
                case CLASS_SHAMAN:    return 0; // Elemental
                case CLASS_MAGE:      return 2; // Frost
                case CLASS_WARLOCK:   return 0; // Affliction
                case CLASS_DRUID:     return 2; // Balance
                default:              return 0;
            }
        }

        std::map<uint8, uint32> tabs = GetTalentPointsPerTab(player);
        uint8 bestTab = 0;
        uint32 bestPoints = 0;
        for (std::map<uint8, uint32>::iterator itr = tabs.begin(); itr != tabs.end(); ++itr)
        {
            if (itr->second > bestPoints)
            {
                bestPoints = itr->second;
                bestTab = itr->first;
            }
        }
        return bestTab;
    }
}

#endif
