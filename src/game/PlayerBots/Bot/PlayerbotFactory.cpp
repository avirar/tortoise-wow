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

    // AC pattern: 10 chars per account (vanilla max)
    const uint32 charsPerAccount = 10;
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

        uint32 charGuid = CreateBotCharacter(accountId);
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

uint32 PlayerbotFactory::CreateBotCharacter(uint32 accountId)
{
    uint32 guid = sObjectMgr.GeneratePlayerLowGuid();

    // Pick random valid race/class combination
    uint32 comboIdx = urand(0, (uint32)s_validRaceClass.size() - 1);
    uint8 race = s_validRaceClass[comboIdx].first;
    uint8 class_ = s_validRaceClass[comboIdx].second;
    uint8 gender = urand(0, 1) ? GENDER_MALE : GENDER_FEMALE;

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
    LOG_DEBUG("playerbots", "[FACTORY] Creating: name='%s' race=%u (%s) class=%u (%s) gender=%u",
        name.c_str(), race, rEntry ? rEntry->name[0] : "?", class_, cEntry ? cEntry->name[0] : "?", gender);

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

    newChar->SetCinematic(1);

    // AC doesn't check SaveToDB return value - transaction layer can return false even on success
    newChar->SaveToDB(true, false);

    delete newChar;
    delete sess;

    sObjectMgr.LoadPlayerCacheData(guid);

    LOG_DEBUG("playerbots", "Playerbot Factory: Created character '%s' (guid=%u, account=%u)", name.c_str(), guid, accountId);
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
