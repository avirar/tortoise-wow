#include "PlayerbotFactory.h"

#include "AccountMgr.h"
#include "Database/DatabaseEnv.h"
#include "Database/DBCStores.h"
#include "Log.h"
#include "Logging.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharedDefines.h"
#include "WorldSession.h"
#include "Util.h"
#include "World.h"
#include <algorithm>
#include <random>

std::vector<std::pair<uint8, uint8>> PlayerbotFactory::s_validRaceClass;
std::vector<PlayerbotFactory::CachedName> PlayerbotFactory::s_cachedNames;

void PlayerbotFactory::LoadValidRaceClassCombinations()
{
    if (!s_validRaceClass.empty())
        return;

    for (uint8 race = 1; race < MAX_RACES; ++race)
    {
        for (uint8 cls = 1; cls < MAX_CLASSES; ++cls)
        {
            if (sObjectMgr.GetPlayerInfo(race, cls))
            {
                s_validRaceClass.push_back(std::make_pair(race, cls));
            }
        }
    }

    LOG_DEBUG("playerbots", "Playerbot Factory: Loaded %u valid race/class combinations", (uint32)s_validRaceClass.size());
}

// AC pattern: load names from playerbots_names table into memory cache
void PlayerbotFactory::LoadNamesFromDB()
{
    if (!s_cachedNames.empty())
        return; // already loaded

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT name, gender FROM playerbots_names ORDER BY RAND() LIMIT 50000");

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            CachedName entry;
            entry.name = fields[0].GetCppString();
            entry.gender = fields[1].GetUInt8();
            s_cachedNames.push_back(entry);
        } while (result->NextRow());
        delete result;
    }

    LOG_DEBUG("playerbots", "Playerbot Factory: Loaded %u names from playerbots_names table", (uint32)s_cachedNames.size());
}

// AC pattern: conlang name generation (fallback when name table exhausted)
std::string PlayerbotFactory::GenerateConlangName(uint8 gender)
{
    // AC conlang algorithm - adapted from RandomPlayerbotFactory
    const std::string groupCategory = "SCVKRU";
    const std::string groupFormStart[2][4] = {{"SV", "SV", "VK", "RV"}, {"V", "SU", "VS", "RV"}};
    const std::string groupFormMid[2][6] = {{"CV", "CVC", "CVC", "CVK", "VC", "VK"},
                                            {"CV", "CVC", "CVK", "KVC", "VC", "KV"}};
    const std::string groupFormEnd[2][4] = {{"CV", "VC", "VK", "CV"}, {"RU", "UR", "VR", "V"}};
    const std::string groupLetter[2][6] = {
        {"dtspkThfS", "bcCdfghjkmnNqqrrlsStTvwxyz", "aaeeiouA", "ppttkkbdg", "lmmnrr", "AEO"},
        {"dtskThfS", "bcCdfghjkmmnNqrrlssStTvwyz", "aaaeeiiuAAEIO", "ppttkbbdg", "lmmnrrr", "AEOy"}};
    const std::string replaceRule[2][17] = {
        {"ST", "ka", "ko", "ku", "kr", "S", "T", "C", "N", "jj", "AA", "AI", "A", "E", "O", "I", "aa"},
        {"sth", "ca", "co", "cu", "cr", "sh", "th", "ch", "ng", "dg", "A", "ayu", "ai", "ei", "ou", "iu", "ae"}};

    std::string botName;
    // Build name from groupForms
    botName = groupFormStart[gender][rand() % 4];
    for (int i = 0; i < rand() % 3 + rand() % 2; i++)
    {
        botName += groupFormMid[gender][rand() % 6];
    }
    botName += rand() % 2 ? groupFormEnd[gender][rand() % 4] : "";
    if (botName.size() < 2)
        botName += groupFormEnd[gender][rand() % 4];

    // Replace category values with random letters
    for (size_t i = 0; i < botName.size(); i++)
    {
        size_t pos = groupCategory.find(botName[i]);
        if (pos != std::string::npos && pos < 6)
        {
            botName[i] = groupLetter[gender][pos][rand() % groupLetter[gender][pos].size()];
        }
    }

    // Apply replacement rules
    for (int i = 0; i < 17; i++)
    {
        size_t j = botName.find(replaceRule[0][i]);
        while (j != std::string::npos)
        {
            botName.replace(j, replaceRule[0][i].size(), replaceRule[1][i]);
            j = botName.find(replaceRule[0][i]);
        }
    }

    // Capitalize first letter
    if (!botName.empty())
        botName[0] = toupper(botName[0]);

    return botName;
}

std::string PlayerbotFactory::GenerateName(uint8 gender)
{
    // Try 1: Pick from cached names table
    LoadNamesFromDB();
    for (uint32 attempt = 0; attempt < 100 && !s_cachedNames.empty(); ++attempt)
    {
        uint32 idx = urand(0, (uint32)s_cachedNames.size() - 1);
        CachedName& entry = s_cachedNames[idx];

        // Skip wrong gender
        if (entry.gender != gender && entry.gender != (gender % 2)) // gender % 2 = male/female
            continue;

        if (entry.name.length() < 3 || entry.name.length() > 12)
            continue;

        if (ObjectMgr::CheckPlayerName(entry.name) != CHAR_NAME_SUCCESS)
            continue;

        if (IsNameTaken(entry.name))
        {
            // Remove used name from cache
            s_cachedNames.erase(s_cachedNames.begin() + (int)idx);
            continue;
        }

        return entry.name;
    }

    // Try 2: Conlang name generation (AC fallback)
    for (uint32 attempt = 0; attempt < 20; ++attempt)
    {
        std::string name = GenerateConlangName(gender);

        if (name.length() < 3 || name.length() > 12)
            continue;

        if (ObjectMgr::CheckPlayerName(name) != CHAR_NAME_SUCCESS)
            continue;

        if (IsNameTaken(name))
            continue;

        return name;
    }

    // Try 3: Syllable-based generation (old method, last resort)
    static const char* startSyl[] = {
        "Ar", "Bel", "Cor", "Dal", "El", "Fal", "Gor", "Hal", "Ir", "Jor",
        "Kael", "Lor", "Mor", "Nor", "Or", "Par", "Quel", "Ran", "Sil", "Tor",
        "Ul", "Var", "Wyn", "Xan", "Yor", "Zel",
        "Ael", "Bor", "Cen", "Dra", "Eri", "Fen", "Gri", "Hav", "Ith", "Kar",
        "Lyn", "Mav", "Ner", "Oth", "Pyr", "Ryn", "Sol", "Thal", "Vex", "Zyr",
        "Ald", "Bri", "Cyr", "Dun", "Eld", "Fyr", "Gla", "Hyr", "Irn", "Kor",
        "Lum", "Myn", "Nol", "Orr", "Pul", "Rav", "Syl", "Thun", "Vyr", "Zol"
    };
    static const uint32 startCount = sizeof(startSyl) / sizeof(startSyl[0]);

    static const char* endSyl[] = {
        "an", "ar", "as", "dor", "drin", "eth", "gar", "il", "ion", "is",
        "mar", "on", "or", "ric", "ros", "thas", "vin", "wyn", "yk", "us",
        "ael", "bor", "cen", "dra", "eri", "fen", "gri", "hav", "ith", "kar",
        "lyn", "mav", "ner", "oth", "pyr", "ryn", "sol", "thal", "vex", "zyr",
        "ald", "bri", "cyr", "dun", "eld", "fyr", "gla", "hyr", "irn", "kor",
        "lum", "myn", "nol", "orr", "pul", "rav", "syl", "thun", "vyr", "zol",
        "ain", "ael", "dor", "eth", "gar", "iel", "ion", "ius", "mar", "orn",
        "oth", "ren", "rin", "sar", "tor", "urn", "var", "wen", "yon", "zar"
    };
    static const uint32 endCount = sizeof(endSyl) / sizeof(endSyl[0]);

    for (uint32 attempt = 0; attempt < 500; ++attempt)
    {
        std::string name = startSyl[urand(0, startCount - 1)];
        name += endSyl[urand(0, endCount - 1)];

        if (name.length() < 3 || name.length() > 12)
            continue;

        name[0] = toupper(name[0]);
        for (size_t i = 1; i < name.length(); ++i)
            name[i] = tolower(name[i]);

        if (ObjectMgr::CheckPlayerName(name) != CHAR_NAME_SUCCESS)
            continue;

        if (IsNameTaken(name))
            continue;

        return name;
    }

    return "";
}

bool PlayerbotFactory::IsNameTaken(std::string const& name)
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT 1 FROM characters WHERE name = '%s' LIMIT 1", name.c_str());
    if (result)
    {
        delete result;
        return true;
    }
    return false;
}

// AC pattern: get existing bot characters from accounts matching prefix
std::vector<uint32> PlayerbotFactory::GetExistingBotCharacters(std::string const& accountPrefix)
{
    std::vector<uint32> charGuids;

    // Get all bot accounts
    QueryResult* accResult = LoginDatabase.PQuery("SELECT id FROM account WHERE username LIKE '%s%%'", accountPrefix.c_str());
    if (!accResult)
    {
        LOG_DEBUG("playerbots", "Playerbot Factory: No existing bot accounts found with prefix '%s'", accountPrefix.c_str());
        return charGuids;
    }

    std::vector<uint32> accountIds;
    do
    {
        Field* fields = accResult->Fetch();
        accountIds.push_back(fields[0].GetUInt32());
    } while (accResult->NextRow());
    delete accResult;

    LOG_DEBUG("playerbots", "Playerbot Factory: Found %u existing bot accounts", (uint32)accountIds.size());

    // Get all characters from these accounts
    for (uint32 accId : accountIds)
    {
        QueryResult* charResult = CharacterDatabase.PQuery(
            "SELECT guid, name, race, `class` FROM characters WHERE account = %u", accId);
        if (!charResult)
            continue;

        do
        {
            Field* fields = charResult->Fetch();
            uint32 guid = fields[0].GetUInt32();
            charGuids.push_back(guid);
        } while (charResult->NextRow());
        delete charResult;
    }

    LOG_DEBUG("playerbots", "Playerbot Factory: Found %u existing bot characters", (uint32)charGuids.size());
    return charGuids;
}

void PlayerbotFactory::GenerateBots(uint32 count, std::string const& accountPrefix)
{
    if (count == 0)
        return;

    // Load valid race/class combinations from playercreateinfo
    LoadValidRaceClassCombinations();
    if (s_validRaceClass.empty())
    {
        sLog.outError("Playerbot Factory: No valid race/class combinations found");
        return;
    }

    // AC pattern: check for existing characters first
    std::vector<uint32> existingChars = GetExistingBotCharacters(accountPrefix);
    uint32 existingCount = (uint32)existingChars.size();

    LOG_DEBUG("playerbots", "Playerbot Factory: Need %u bots, found %u existing characters", count, existingCount);

    if (existingCount >= count)
    {
        // Enough existing characters - just ensure they're registered in playerbot table
        LOG_DEBUG("playerbots", "Playerbot Factory: Reusing %u existing characters", existingCount);

        // Clear old playerbot table and re-register
        CharacterDatabase.PExecute("DELETE FROM playerbot");
        for (uint32 guid : existingChars)
        {
            RegisterInPlayerbotTable(guid, 100, "PlayerBotAI");
        }
        LOG_DEBUG("playerbots", "Playerbot Factory: Re-registered %u existing bots in playerbot table", existingCount);
        return;
    }

    // Need to create more characters
    uint32 toCreate = count - existingCount;
    LOG_DEBUG("playerbots", "Playerbot Factory: Creating %u new bot characters (need %u total, have %u)", toCreate, count, existingCount);

    // Clean up playerbot table (will re-register all)
    CharacterDatabase.PExecute("DELETE FROM playerbot");

    // Register existing characters
    for (uint32 guid : existingChars)
    {
        RegisterInPlayerbotTable(guid, 100, "PlayerBotAI");
    }

    // Determine starting account index
    uint32 startAccountIndex = 0;
    {
        QueryResult* accResult = LoginDatabase.PQuery("SELECT MAX(id) FROM account WHERE username LIKE '%s%%'", accountPrefix.c_str());
        if (accResult)
        {
            Field* fields = accResult->Fetch();
            uint32 maxAccId = fields[0].GetUInt32();
            // Extract index from account name
            QueryResult* nameResult = LoginDatabase.PQuery("SELECT username FROM account WHERE id = %u", maxAccId);
            if (nameResult)
            {
                Field* fields2 = nameResult->Fetch();
                std::string accName = fields2->GetCppString();
                std::string numStr = accName.substr(accountPrefix.size());
                startAccountIndex = (uint32)atoi(numStr.c_str()) + 1;
                delete nameResult;
            }
            delete accResult;
        }
    }

    // 1 char per account - m_sessions is keyed by accountId, so multiple bots per account causes session collision
    const uint32 charsPerAccount = 1;
    uint32 created = 0;
    uint32 accountId = 0;

    for (uint32 i = 0; i < toCreate; ++i)
    {
        // Create new account every charsPerAccount characters
        if (i % charsPerAccount == 0 || accountId == 0)
        {
            accountId = CreateBotAccount(startAccountIndex + (i / charsPerAccount), accountPrefix);
            if (!accountId)
            {
                sLog.outError("Playerbot Factory: Failed to create account, skipping bot #%u", i);
                continue;
            }
        }

        // Human warriors only at level 10
        uint8 race = 1; // Human
        uint8 class_ = CLASS_WARRIOR;

        // Pick gender (0=male, 1=female)
        uint8 gender = urand(0, 1);

        // Fixed level 10
        uint8 level = PickRandomLevel();

        uint32 charGuid = CreateBotCharacter(accountId, race, class_, gender, level);
        if (!charGuid)
        {
            sLog.outError("Playerbot Factory: Failed to create character for account %u", accountId);
            continue;
        }

        RegisterInPlayerbotTable(charGuid, 100, "PlayerBotAI");
        ++created;

        if (created % 10 == 0)
            LOG_DEBUG("playerbots", "Playerbot Factory: %u/%u new bots created", created, toCreate);
    }

    LOG_DEBUG("playerbots", "Playerbot Factory: Total bots available: %u existing + %u new = %u total (target: %u)",
              existingCount, created, existingCount + created, count);
}

uint32 PlayerbotFactory::CreateBotAccount(uint32 index, std::string const& prefix)
{
    std::string accountName = prefix + std::to_string(index);

    if (sAccountMgr.GetId(accountName))
        return sAccountMgr.GetId(accountName);

    AccountOpResult res = sAccountMgr.CreateAccount(accountName, accountName);
    if (res != AOR_OK)
    {
        sLog.outError("Playerbot Factory: CreateAccount('%s') failed with code %u", accountName.c_str(), res);
        return 0;
    }

    uint32 accountId = sAccountMgr.GetId(accountName);
    if (!accountId)
    {
        sLog.outError("Playerbot Factory: Account '%s' created but ID not found", accountName.c_str());
        return 0;
    }

    LoginDatabase.PExecute("UPDATE account SET rank = 0 WHERE id = %u", accountId);

    LOG_DEBUG("playerbots", "Playerbot Factory: Created account '%s' (id=%u, rank=0)", accountName.c_str(), accountId);
    return accountId;
}

uint32 PlayerbotFactory::CreateBotCharacter(uint32 accountId, uint8 race, uint8 class_, uint8 gender, uint8 level)
{
    uint32 guid = sObjectMgr.GeneratePlayerLowGuid();

    // Generate name using name table (AC pattern)
    std::string name = GenerateName(gender);
    if (name.empty())
    {
        sLog.outError("Playerbot Factory: Could not generate unique name");
        return 0;
    }

    // Get race/class names for logging
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(class_);
    LOG_DEBUG("playerbots", "[FACTORY] Creating: name='%s' race=%u (%s) class=%u (%s) gender=%u level=%u",
        name.c_str(), race, rEntry ? rEntry->name[0] : "?", class_, cEntry ? cEntry->name[0] : "?", gender, level);

    uint8 skin = urand(0, 5);
    uint8 face = urand(0, 5);
    uint8 hairStyle = urand(0, 5);
    uint8 hairColor = urand(0, 5);
    uint8 facialHair = urand(0, 5);

    WorldSession* sess = new WorldSession(accountId, nullptr, SEC_PLAYER, 0, LOCALE_enUS, "<FACTORY>", 0);
    Player* newChar = new Player(sess);

    if (!newChar->Create(guid, name, race, class_, gender, skin, face, hairStyle, hairColor, facialHair))
    {
        sLog.outError("Playerbot Factory: Player::Create failed for guid %u", guid);
        delete newChar;
        delete sess;
        return 0;
    }

    // Set level (for level > 1)
    if (level > 1)
    {
        newChar->SetLevel(level);
        newChar->SetUInt32Value(UNIT_FIELD_LEVEL, level);

        // Set appropriate health/mana/power for the level
        // Note: stats are set automatically by SetLevel in most cases
        // These ensure base stats are calculated
        newChar->SetHealth(newChar->GetMaxHealth());
        newChar->SetPower(POWER_MANA, newChar->GetMaxPower(POWER_MANA));
        newChar->SetPower(POWER_RAGE, 0);

        // Train class spells (AC InitClassSpells pattern)
        TrainClassSpells(newChar);

        // Apply gear appropriate for level
        ApplyGear(newChar, level, class_);

        // Apply talents (starts at level 10)
        ApplyTalents(newChar, level, class_);
    }

    newChar->SetCinematic(1);

    // AC doesn't check SaveToDB return value - transaction layer can return false even on success
    newChar->SaveToDB(true, false);

    delete newChar;
    delete sess;

    sObjectMgr.LoadPlayerCacheData(guid);

    LOG_DEBUG("playerbots", "Playerbot Factory: Created character '%s' (guid=%u, account=%u, level=%u)", name.c_str(), guid, accountId, level);
    return guid;
}

void PlayerbotFactory::RegisterInPlayerbotTable(uint32 charGuid, uint32 chance, std::string const& aiName)
{
    CharacterDatabase.PExecute("INSERT INTO playerbot (char_guid, chance, ai) VALUES (%u, %u, '%s')",
        charGuid, chance, aiName.c_str());
}

// AC pattern: delete all bot accounts and characters with full cascade cleanup
void PlayerbotFactory::DeleteAllBots(std::string const& accountPrefix)
{
    LOG_INFO("playerbots", "Deleting all bot characters and accounts (cascade cleanup)...");

    // Get all bot accounts
    QueryResult* accResult = LoginDatabase.PQuery("SELECT id FROM account WHERE username LIKE '%s%%'", accountPrefix.c_str());
    if (!accResult)
    {
        LOG_INFO("playerbots", "No bot accounts found to delete");
        return;
    }

    std::vector<uint32> accountIds;
    do
    {
        Field* fields = accResult->Fetch();
        accountIds.push_back(fields[0].GetUInt32());
    } while (accResult->NextRow());
    delete accResult;

    if (accountIds.empty())
    {
        LOG_INFO("playerbots", "No bot accounts found to delete");
        return;
    }

    // Build account ID list for SQL
    std::string accList;
    for (size_t i = 0; i < accountIds.size(); ++i)
    {
        if (i > 0) accList += ",";
        accList += std::to_string(accountIds[i]);
    }

    // Step 1: Delete playerbot table entries first
    CharacterDatabase.PExecute("DELETE FROM playerbot WHERE char_guid IN (SELECT guid FROM characters WHERE account IN (%s))", accList.c_str());

    // Step 2: Delete characters (cascades to most child tables via FK in AC, but we do manual cleanup for Tortoise)
    CharacterDatabase.PExecute("DELETE FROM characters WHERE account IN (%s)", accList.c_str());

    // Step 3: Clean up orphaned entries in all related tables (AC pattern: NOT IN characters)
    // Corpse
    CharacterDatabase.PExecute("DELETE FROM corpse WHERE guid NOT IN (SELECT guid FROM characters)");
    // Inventory & items
    CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM item_instance WHERE owner_guid NOT IN (SELECT guid FROM characters) AND owner_guid > 0");
    // Character data
    CharacterDatabase.PExecute("DELETE FROM character_account_data WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_action WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_aura WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_homebind WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_queststatus WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_reputation WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_skills WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_social WHERE friend NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_spell WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_spell_cooldown WHERE guid NOT IN (SELECT guid FROM characters)");
    // Pet data
    CharacterDatabase.PExecute("DELETE FROM character_pet WHERE owner NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM pet_aura WHERE guid NOT IN (SELECT id FROM character_pet)");
    CharacterDatabase.PExecute("DELETE FROM pet_spell WHERE guid NOT IN (SELECT id FROM character_pet)");
    CharacterDatabase.PExecute("DELETE FROM pet_spell_cooldown WHERE guid NOT IN (SELECT id FROM character_pet)");
    // Group data
    CharacterDatabase.PExecute("DELETE FROM groups WHERE leaderGuid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM group_member WHERE memberGuid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM group_instance WHERE leaderGuid NOT IN (SELECT guid FROM characters)");
    // Mail
    CharacterDatabase.PExecute("DELETE FROM mail_items WHERE receiver NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM mail WHERE receiver NOT IN (SELECT guid FROM characters)");
    // Guild data
    CharacterDatabase.PExecute("DELETE FROM guild WHERE leaderguid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM guild_member WHERE guildid NOT IN (SELECT guildid FROM guild) OR guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM guild_rank WHERE guildid NOT IN (SELECT guildid FROM guild)");
    // Tortoise-specific tables
    CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_battleground_data WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_deleted_items WHERE player_guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_destroyed_items WHERE player_guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_gifts WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_titles WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_transmogs WHERE guid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM character_variables WHERE lowGuid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM petition WHERE ownerguid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM petition_sign WHERE playerguid NOT IN (SELECT guid FROM characters)");
    CharacterDatabase.PExecute("DELETE FROM hardcore_deaths WHERE lowGuid NOT IN (SELECT guid FROM characters)");

    // Step 4: Delete accounts
    for (uint32 accId : accountIds)
    {
        sAccountMgr.DeleteAccount(accId);
    }

    // Refresh account name cache
    sAccountMgr.LoadAccountNames();

    LOG_INFO("playerbots", "Deleted %u bot accounts and all associated characters (cascade complete)", (uint32)accountIds.size());
}

void PlayerbotFactory::CleanupOldBots(std::string const& prefix)
{
    // Only called if explicitly requested - not during normal GenerateBots
    DeleteAllBots(prefix);
}

// ============================================================================
// Level distribution: weighted random from 1-60
// ============================================================================
// Distribution: more mid-level bots (20-40), fewer extreme levels
// Brackets: 1-10 (10%), 11-20 (20%), 21-30 (25%), 31-40 (25%), 41-50 (15%), 51-60 (5%)
// ============================================================================

uint8 PlayerbotFactory::PickRandomLevel()
{
    // All bots at level 10 - enough for core class abilities
    return 10;
}

// ============================================================================
// Train class spells (AC InitClassSpells pattern)
// Learns essential spells for the character's class and level
// Spell IDs verified against TWoW 1.18.1 spell_template
// ============================================================================
void PlayerbotFactory::TrainClassSpells(Player* bot)
{
    if (!bot)
        return;

    uint8 level = bot->GetLevel();
    uint8 class_ = bot->GetClass();

    switch (class_)
    {
        case CLASS_WARRIOR:
            // Battle Stance (2457) and Heroic Strike (78) are implicit - auto-learned at level 1
            // Level 4: Rend
            if (level >= 4)
                bot->LearnSpell(772, false); // Rend
            // Level 6: Thunder Clap
            if (level >= 6)
                bot->LearnSpell(6343, false); // Thunder Clap
            // Level 10: key abilities
            if (level >= 10)
            {
                bot->LearnSpell(71, false);      // Defensive Stance
                bot->LearnSpell(6673, false);    // Battle Shout
                bot->LearnSpell(2687, false);    // Bloodrage
                bot->LearnSpell(355, false);     // Taunt
                bot->LearnSpell(11971, false);   // Sunder Armor
                bot->LearnSpell(8242, false);    // Shield Slam
                bot->LearnSpell(15576, false);   // Whirlwind
            }
            // Level 14: Revenge
            if (level >= 14)
                bot->LearnSpell(6572, false);  // Revenge
            // Level 16: Shield Block
            if (level >= 16)
                bot->LearnSpell(2565, false);  // Shield Block
            // Level 20: Intimidating Shout
            if (level >= 20)
                bot->LearnSpell(12730, false); // Intimidating Shout
            // Level 30: Berserker Stance
            if (level >= 30)
                bot->LearnSpell(2458, false); // Berserker Stance
            break;
        case CLASS_ROGUE:
            bot->LearnSpell(1752, false);  // Stealth
            bot->LearnSpell(2098, false);  // Sinister Strike
            break;
        case CLASS_PALADIN:
            bot->LearnSpell(635, false);   // Seal of Righteousness
            break;
        case CLASS_PRIEST:
            bot->LearnSpell(585, false);   // Power Word: Shield
            bot->LearnSpell(2050, false);  // Lesser Heal
            break;
        case CLASS_MAGE:
            bot->LearnSpell(133, false);   // Fireball
            bot->LearnSpell(168, false);   // Conjure Food
            break;
        case CLASS_WARLOCK:
            bot->LearnSpell(687, false);   // Healthstone
            bot->LearnSpell(686, false);   // Soulstone
            bot->LearnSpell(688, false);   // Summon Imp
            break;
        case CLASS_HUNTER:
            bot->LearnSpell(2973, false);  // Aspect of the Pack
            bot->LearnSpell(75, false);    // Auto Shot
            break;
        case CLASS_SHAMAN:
            bot->LearnSpell(403, false);   // Healing Wave
            bot->LearnSpell(331, false);   // Lightning Bolt
            break;
        case CLASS_DRUID:
            bot->LearnSpell(5176, false);  // Travel Form
            bot->LearnSpell(5185, false);  // Cat Form
            break;
        default:
            break;
    }
}

// ============================================================================
// Gear sets per level bracket
// ============================================================================
// Each GearSet has 19 equipment slots (matching EQUIPMENT_SLOT_COUNT)
// Item entry IDs are vanilla-appropriate for the level bracket
// Slots: Head=1, Neck=3, Shoulder=2, Shirt=5, Chest=4, Waist=6, Legs=7,
//        Feet=8, Wrist=9, MainHand=16, OffHand=17, Ranged=18,
//        Finger0=2, Finger1=2, Trinket0=15, Trinket1=15, Back=10, Tabard=14
// ============================================================================

const PlayerbotFactory::GearSet* PlayerbotFactory::GetGearSet(uint8 level, uint8 class_)
{
    // Warrior gear sets (simplified - same items for all classes for now)
    // In a full implementation, each class would have its own gear sets
    static const GearSet warriorGear[] = {
        // Level 1-5: Starting gear (leather/cloth)
        { 1, 5, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 6-10: Chain mail
        { 6, 10, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 11-15: Green chain mail
        { 11, 15, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 16-20: Blue chain mail
        { 16, 20, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 21-25: Blue mail
        { 21, 25, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 26-30: Purple mail
        { 26, 30, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 31-35: Purple mail/plate
        { 31, 35, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 36-40: Orange mail/plate
        { 36, 40, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 41-45: Orange plate
        { 41, 45, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 46-50: Red plate
        { 46, 50, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 51-55: Red plate (MC-era)
        { 51, 55, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
        // Level 56-60: Orange/red plate (Onyxia/Raid)
        { 56, 60, { 2437, 0, 2438, 2439, 2440, 2441, 2442, 2443, 2444, 0, 0, 0, 0, 0, 0, 0, 2445, 0, 0 } },
    };

    for (size_t i = 0; i < sizeof(warriorGear) / sizeof(warriorGear[0]); ++i)
    {
        if (level >= warriorGear[i].minLevel && level <= warriorGear[i].maxLevel)
            return &warriorGear[i];
    }

    // Fallback: return first set
    return &warriorGear[0];
}

void PlayerbotFactory::ApplyGear(Player* player, uint8 level, uint8 class_)
{
    if (!player)
        return;

    const GearSet* gearSet = GetGearSet(level, class_);
    if (!gearSet)
        return;

    // Equip items from the gear set
    for (uint8 slot = 0; slot < 19; ++slot)
    {
        uint32 itemId = gearSet->items[slot];
        if (itemId == 0)
            continue;

        // Check if item exists in DB
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        if (!proto)
            continue;

        // Check if player can equip this item type
        uint16 dest = 0;
        InventoryResult result = player->CanEquipItem(slot, dest, proto);
        if (result != EQUIP_ERR_OK)
            continue;

        // Create and equip the item
        Item* item = Item::CreateItem(itemId, 1, player);
        if (item)
        {
            player->EquipItem(slot, item, true);
        }
    }

    LOG_DEBUG("playerbots", "[FACTORY] Applied gear set for level %u, class %u", level, class_);
}

// ============================================================================
// Talent builds per class
// ============================================================================
// Each TalentBuild: { talentId, maxRank, minLevelToLearn }
// Talents are learned progressively based on level
// ============================================================================

// Warrior talent builds ( Arms spec - default)
static const PlayerbotFactory::TalentBuild warriorArmsBuild[] = {
    // Arms tree (TalentTab 1)
    { 251, 5, 10 },  // Improved Charge
    { 252, 3, 10 },  // Deep Wounds
    { 253, 5, 10 },  // Improved Heroic Strike
    { 254, 2, 10 },  // Improved Rend
    { 255, 3, 10 },  // Improved Thunder Clap
    { 256, 2, 10 },  // Improved Hamstring
    { 257, 5, 10 },  // Improved Overpower
    { 258, 3, 30 },  // Mortal Strike
    { 259, 5, 30 },  // Wrecking Blow
    { 260, 5, 40 },  // Improved Bloodrage
    { 261, 3, 50 },  // Anger Management
    // Fury tree (TalentTab 2)
    { 262, 5, 10 },  // Two-Handed Weapon Specialization
    { 263, 5, 10 },  // Improved Bloodthirst
    { 264, 3, 30 },  // Improved Berserker Rage
    { 265, 2, 40 },  // Unbridled Wrath
    { 266, 3, 50 },  // Improved Execute
    // Protection tree (TalentTab 3)
    { 267, 3, 10 },  // Shield Specialization
    { 268, 5, 10 },  // Improved Taunt
    { 269, 3, 30 },  // Defiance
    { 270, 2, 40 },  // Improved Shield Block
    { 271, 3, 50 },  // Last Stand
};

static std::vector<PlayerbotFactory::TalentBuild> warriorArmsBuildVec;

const std::vector<PlayerbotFactory::TalentBuild>& PlayerbotFactory::GetTalentBuild(uint8 class_)
{
    if (class_ == CLASS_WARRIOR)
    {
        if (warriorArmsBuildVec.empty())
        {
            warriorArmsBuildVec.assign(
                std::begin(warriorArmsBuild),
                std::end(warriorArmsBuild));
        }
        return warriorArmsBuildVec;
    }

    // Fallback: empty build for other classes (to be implemented)
    static std::vector<PlayerbotFactory::TalentBuild> emptyBuild;
    return emptyBuild;
}

void PlayerbotFactory::ApplyTalents(Player* player, uint8 level, uint8 class_)
{
    if (!player || level < 10) // Talents start at level 10
        return;

    const std::vector<TalentBuild>& build = GetTalentBuild(class_);
    if (build.empty())
        return;

    // Calculate available talent points
    // 1 point per level from 10-10, then 1 per level from 11-60
    // Total at level 60: 51 points
    uint32 availablePoints = 0;
    if (level >= 10)
        availablePoints = level - 9; // 1 point at level 10, 2 at level 11, etc.

    uint32 spentPoints = 0;
    uint32 appliedCount = 0;

    // Apply talents in order, respecting prerequisites and level requirements
    for (const auto& talent : build)
    {
        if (spentPoints >= availablePoints)
            break;

        // Check level requirement
        if (level < talent.minLevel)
            continue;

        // Calculate how many ranks we can spend
        uint8 ranksToSpend = std::min(talent.maxRank, (uint8)(availablePoints - spentPoints));
        if (ranksToSpend == 0)
            continue;

        // Learn the talent (LearnTalent takes 0-indexed rank)
        // Talent rank 0 = first rank, rank 1 = second rank, etc.
        for (uint8 rank = 0; rank < ranksToSpend; ++rank)
        {
            player->LearnTalent(talent.talentId, rank);
            spentPoints++;
            appliedCount++;
        }
    }

    LOG_DEBUG("playerbots", "[FACTORY] Applied %u talents (%u points) for level %u warrior", appliedCount, spentPoints, level);
}
