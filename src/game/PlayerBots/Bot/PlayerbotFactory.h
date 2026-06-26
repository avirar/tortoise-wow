#ifndef _PLAYERBOT_FACTORY_H
#define _PLAYERBOT_FACTORY_H

#include "Common.h"
#include <vector>
#include <string>
#include <map>

class Player;

class PlayerbotFactory
{
public:
    /** Generate bots: creates accounts and characters if needed, registers in playerbot table.
     *  On subsequent runs, detects existing characters and reuses them.
     *  Bots are generated across level 1-60 with appropriate gear and talents. */
    static void GenerateBots(uint32 count, std::string const& accountPrefix);

    /** Get all bot characters (existing + newly created), returns list of GUIDs */
    static std::vector<uint32> GetExistingBotCharacters(std::string const& accountPrefix);

    /** Delete all bot accounts and characters (AC pattern) */
    static void DeleteAllBots(std::string const& accountPrefix);

private:
    static uint32 CreateBotAccount(uint32 index, std::string const& prefix);
    static uint32 CreateBotCharacter(uint32 accountId, uint8 race, uint8 class_, uint8 gender, uint8 level);
    static std::string GenerateName(uint8 gender);
    static bool IsNameTaken(std::string const& name);
    static void RegisterInPlayerbotTable(uint32 charGuid, uint32 chance, std::string const& aiName);
    static void CleanupOldBots(std::string const& prefix);

    // Load valid race/class combinations from playercreateinfo
    static void LoadValidRaceClassCombinations();
    static std::vector<std::pair<uint8, uint8>> s_validRaceClass;

    // AC pattern: load names from playerbots_names table into memory cache
    static void LoadNamesFromDB();
    struct CachedName { std::string name; uint8 gender; };
    static std::vector<CachedName> s_cachedNames;

    // AC pattern: conlang name generation fallback
    static std::string GenerateConlangName(uint8 gender);

    // Level distribution: pick a random level from 1-60 with weighted distribution
    static uint8 PickRandomLevel();

    // Gear and talent application (called after character creation)
    static void ApplyGear(Player* player, uint8 level, uint8 class_);
    static void ApplyTalents(Player* player, uint8 level, uint8 class_);
    static void TrainClassSpells(Player* player);

    // Gear sets per level bracket (item entry IDs)
    struct GearSet { uint8 minLevel; uint8 maxLevel; uint32 items[19]; };
    static const GearSet* GetGearSet(uint8 level, uint8 class_);

public:
    // Talent builds per class (talentId -> rank mapping)
    // Each entry: { talentId, rankAtLevel60, minLevelToLearn }
    // Public so it can be used at file scope in .cpp
    struct TalentBuild { uint32 talentId; uint8 maxRank; uint8 minLevel; };
    static const std::vector<TalentBuild>& GetTalentBuild(uint8 class_);
};

#endif
