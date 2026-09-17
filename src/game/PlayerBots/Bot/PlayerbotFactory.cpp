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
#include "Maps/MapManager.h"
#include <algorithm>
#include <random>
#include <cmath>

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

        // Spread bots across all 10 races (Alliance + Horde)
        static const uint8 allRaces[] = {
            RACE_HUMAN,     // 1 - Goldshire
            RACE_ORC,       // 2 - Razor Hill
            RACE_DWARF,     // 3 - Kharanos
            RACE_NIGHTELF,  // 4 - Dolanaar
            RACE_UNDEAD,    // 5 - Brill
            RACE_TAUREN,    // 6 - Bloodhoof Village
            RACE_GNOME,     // 7 - Kharanos
            RACE_TROLL,     // 8 - Razor Hill
            RACE_GOBLIN,    // 9 - near Teste
            RACE_HIGH_ELF   // 10 - near Testy
        };
        uint8 race = allRaces[i % 10];
        // playerbot-engine-port R2: pick a random class valid for this race
        // (s_validRaceClass is loaded from playercreateinfo; previously
        // hardcoded CLASS_WARRIOR — all 100 bots were warriors)
        uint8 class_ = CLASS_WARRIOR;
        {
            std::vector<uint8> validClasses;
            for (const auto& rc : s_validRaceClass)
                if (rc.first == race)
                    validClasses.push_back(rc.second);
            if (!validClasses.empty())
                class_ = validClasses[urand(0, (uint32)validClasses.size() - 1)];
        }

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

        // Train armor/weapon skills (bots can't equip without these)
        TrainClassSkills(newChar);

        // Apply a full level-appropriate gear set (generated from item_template).
        // Bots need this to fight mobs of their level — without gear, melee
        // classes deal 1 dmg/hit and no class survives level-appropriate mobs
        // past ~12-15, so the grind/loot/gold economy can't work.
        ApplyGear(newChar, level, class_);

        // Apply talents (starts at level 10)
        ApplyTalents(newChar, level, class_);
    }

    // R5: place the bot in a level/faction-appropriate zone so the world has
    // level 1-60 bots spread across both continents (not all at the race's
    // starting village). Mirrors the SetLocationMapId/Relocate/SetMap pattern
    // used by Player::Create. Server corrects slightly-off points to ground.
    {
        BotSpawnPoint sp = PickSpawnPosition(level, race);
        // Defense-in-depth: if the (jittered) point is an invalid map coord,
        // fall back to the band center without jitter so we never crash in
        // Relocate (IsValidMapCoord throws std::runtime_error on bad coords).
        if (!MapManager::IsValidMapCoord(sp.map, sp.x, sp.y, sp.z))
        {
            sLog.outError("playerbots: invalid spawn point (%d, %.0f, %.0f, %.0f) for level %u; using race start fallback", sp.map, sp.x, sp.y, sp.z, level);
            sp = { 0, -6200.f, 300.f, 40.f };
        }
        newChar->SetLocationMapId(sp.map);
        newChar->Relocate(sp.x, sp.y, sp.z, 0.0f);
        if (sp.map <= 1)
            newChar->SetLocationInstanceId(sMapMgr.GetContinentInstanceId(sp.map, sp.x, sp.y));
        newChar->SetMap(sMapMgr.CreateMap(sp.map, newChar));
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
    // R5: distribute bots across the full 1-60 range so the world has
    // level-1..60 players of every class. Mild low-level bias (exponent 1.6)
    // for a natural population shape; ceil keeps 60 as a real top-of-range
    // bucket (not a single-needle roll).
    double r = (double)urand(0, 99999) / 99999.0;   // [0, 0.99999]
    uint32 level = (uint32)std::ceil(60.0 * pow(r, 1.6));
    if (level < 1)   level = 1;
    if (level > 60)  level = 60;
    return (uint8)level;
}

PlayerbotFactory::BotSpawnPoint PlayerbotFactory::PickSpawnPosition(uint8 level, uint8 race)
{
    // Faction: Alliance -> Eastern Kingdoms (map 0), Horde -> Kalimdor (map 1).
    // High levels (50+) converge on the shared contested endgame zone (Silithus).
    bool horde = (race == RACE_ORC || race == RACE_UNDEAD || race == RACE_TAUREN ||
                  race == RACE_TROLL || race == RACE_GOBLIN);

    // Level bands, sorted high -> low: {minLevel, ek(map0), km(map1)}
    // Each point is a REAL HUMANOID (creature type 7) spawn position in a
    // HOSTILE, level-matched humanoid cluster for that band/continent
    // (verified against tw_world.creature, 2026-09-17 — 2nd pass).
    // Humanoids drop gold + equipment; beasts (type 1) drop neither — so
    // spawning in humanoid-dense zones is what feeds the bot economy.
    // Selection criteria (fixes from the 1st pass):
    //   * hostile-only: city/friendly NPCs excluded (faction blacklist + name
    //     eyeball) — the 1st pass counted friendly city humanoids (e.g. the
    //     Elwynn point sat next to the Stormwind gate where EVERY humanoid is
    //     friendly, so bots saw humanoids=0 and suicide-looped the elite-50
    //     Sewer Beast, the only attackable creature there);
    //   * level-matched: mob level_min within [band_low, band_high-4] so the
    //     low half of each band isn't fighting +8..+14 mobs (Razorfen 33-34
    //     at the old KM-20 spot, Spitelash 51-52 at KM-40, Muckshell 39-43
    //     at KM-30, Venture 9-10 at KM-1);
    //   * non-elite (rank=0) and dense (n>=8 in a 150u cell).
    // A real creature position (not the cell centroid) is used so the point
    // is guaranteed on land. Jitter is added in the caller.
    struct Band { uint8 min; BotSpawnPoint ek; BotSpawnPoint km; };
    static const Band bands[] = {
        // 50-60: EK Hearthglen Scarlets 54-57 (n=53) / KM Lucid Dream 55 (n=42)
        { 50, {0,   1783.f,  -5755.f,  116.f}, {1,   6904.f,  -5685.f,   -4.f} },
        // 40-49: EK Bloodsail pirates 43 (n=28) / KM Southsea pirates 44-45 (n=35)
        { 40, {0, -15025.f,    262.f,    8.f}, {1,  -8088.f,  -5245.f,    2.f} },
        // 30-39: EK Bloodscalp trolls 34 (n=33) / KM Burning Blade 31-33 (n=29)
        { 30, {0, -11645.f,    664.f,   50.f}, {1,   -451.f,   1743.f,  147.f} },
        // 20-29: EK Shadowhide gnolls 24-25 (n=34) / KM Windshear 21 (n=47)
        { 20, {0,  -9261.f,  -3283.f,  113.f}, {1,    959.f,   -359.f,   16.f} },
        // 10-19: EK Westfall Defias 14-16 (n=45) / KM Venture Co. 14 (n=28)
        { 10, {0, -10950.f,   1486.f,   37.f}, {1,   1035.f,  -3088.f,  105.f} },
        // 1-9: EK Northshire kobolds 1-3 (n=50) / KM Durotar 5 (n=21)
        {  1, {0,  -8774.f,   -184.f,   83.f}, {1,   -112.f,  -7858.f,   40.f} },
    };
    BotSpawnPoint sp{};
    for (const Band& b : bands)
    {
        if (level >= b.min)
        {
            sp = horde ? b.km : b.ek;
            break;
        }
    }
    // If no band matched (shouldn't happen; level is clamped to 1-60), use the fallback.
    if (sp.map == 0 && sp.x == 0.0f && sp.y == 0.0f)
        sp = { 0, -6200.f, 300.f, 40.f };

    // R5: spawn jitter (+/-400u random offset) so 300 bots don't all pile onto the
    // exact dense cluster center (which would concentrate them and spike target
    // contention). Bots start spread over a ~1600u-diameter area; the wander +
    // 150yd sight distance carries them into the surrounding mobs.
    // NOTE: urand() returns unsigned, so cast to signed int BEFORE subtracting
    // (uint32 underflow on urand<400 produced x=4.29e9 -> IsValidMapCoord throw).
    sp.x += (float)((int)urand(0, 800) - 400);
    sp.y += (float)((int)urand(0, 800) - 400);
    return sp;
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
// Armor/Weapon skill training
// Bots can't equip items without the appropriate skills
// Skill values from ItemPrototype::GetProficiencySkill() (Item.cpp:654)
// Skill cap: level * 5, min 75 for levels 1-9, max 300
// ============================================================================
void PlayerbotFactory::TrainClassSkills(Player* bot)
{
    if (!bot)
        return;

    uint8 level = bot->GetLevel();
    uint8 class_ = bot->GetClass();

    // Vanilla WoW skill cap: level * 5, max 300. Matches the core's
    // GetSkillMaxForLevel() (level * 5) so creation-time values agree with
    // UpdateSkillsForLevel(), which re-maximizes on login and every level-up
    // when AlwaysMaxSkillForLevel is enabled in mangosd.conf.
    uint16 skillVal = std::min<uint16>(level * 5, 300);

    // Helper lambda to set a skill
    auto setSkill = [&bot, skillVal](uint16 skill) {
        bot->SetSkill(skill, skillVal, skillVal);
    };

    // Defense: every class. The bot's own defense skill drives its crit
    // vulnerability (and parry/dodge/block), so it must exist and stay maxed
    // like the weapon skills.
    setSkill(SKILL_DEFENSE);            // 95 minLvl=0

    switch (class_)
    {
        // DBC-verified from SkillRaceClassInfo.dbc (node-dbc-reader)
        // Bit mapping: (1 << (class_id - 1)) from SharedDefines.h + DBCStores.cpp:642
        // CLASS_WARRIOR=1(1), PALADIN=2(2), HUNTER=3(4), ROGUE=4(8), PRIEST=5(16),
        //   SHAMAN=7(64), MAGE=8(128), WARLOCK=9(256), DRUID=11(1024)

        case CLASS_WARRIOR: {
            // Armor (all types): Plate(40), Mail, Leather, Cloth
            setSkill(SKILL_PLATE_MAIL);     // 293 minLvl=40
            setSkill(SKILL_MAIL);           // 413 minLvl=0
            setSkill(SKILL_LEATHER);        // 414 minLvl=0
            setSkill(SKILL_CLOTH);          // 415 minLvl=0
            // Weapons: Swords, Axes, Maces, 2H Swords, 2H Maces, 2H Axes,
            //   Staves, Polearms(20), Daggers, Thrown, Bows, Guns, Crossbows, Shield, Fist
            setSkill(SKILL_SWORDS);         // 43 minLvl=0
            setSkill(SKILL_AXES);           // 44 minLvl=0
            setSkill(SKILL_MACES);          // 54 minLvl=0
            setSkill(SKILL_2H_SWORDS);      // 55 minLvl=0
            setSkill(SKILL_2H_MACES);       // 160 minLvl=0
            setSkill(SKILL_2H_AXES);        // 172 minLvl=0
            setSkill(SKILL_STAVES);         // 136 minLvl=0
            setSkill(SKILL_POLEARMS);       // 229 minLvl=20
            setSkill(SKILL_DAGGERS);        // 173 minLvl=0
            setSkill(SKILL_THROWN);         // 176 minLvl=0
            setSkill(SKILL_BOWS);           // 45 minLvl=0
            setSkill(SKILL_GUNS);           // 46 minLvl=0
            setSkill(SKILL_CROSSBOWS);      // 226 minLvl=0
            setSkill(SKILL_SHIELD);         // 433 minLvl=0
            setSkill(SKILL_FIST_WEAPONS);   // 473 minLvl=0
            break;
        }

        case CLASS_PALADIN: {
            // Armor (all types): Plate(40), Mail, Leather, Cloth
            setSkill(SKILL_PLATE_MAIL);     // 293 minLvl=40
            setSkill(SKILL_MAIL);           // 413 minLvl=0
            setSkill(SKILL_LEATHER);        // 414 minLvl=0
            setSkill(SKILL_CLOTH);          // 415 minLvl=0
            // Weapons: Swords, Axes, Maces, 2H Swords, 2H Maces, 2H Axes, Polearms(20), Shield
            setSkill(SKILL_SWORDS);         // 43 minLvl=0
            setSkill(SKILL_AXES);           // 44 minLvl=0
            setSkill(SKILL_MACES);          // 54 minLvl=0
            setSkill(SKILL_2H_SWORDS);      // 55 minLvl=0
            setSkill(SKILL_2H_MACES);       // 160 minLvl=0
            setSkill(SKILL_2H_AXES);        // 172 minLvl=0
            setSkill(SKILL_POLEARMS);       // 229 minLvl=20
            setSkill(SKILL_SHIELD);         // 433 minLvl=0
            break;
        }

        case CLASS_HUNTER: {
            // Armor: Mail(40), Leather, Cloth
            setSkill(SKILL_MAIL);           // 413 minLvl=40
            setSkill(SKILL_LEATHER);        // 414 minLvl=0
            setSkill(SKILL_CLOTH);          // 415 minLvl=0
            // Weapons: Bows, Guns, Crossbows, Axes, Swords, 2H Swords, 2H Axes,
            //   Daggers, Staves, Thrown, Polearms(20), Fist
            setSkill(SKILL_BOWS);           // 45 minLvl=0
            setSkill(SKILL_GUNS);           // 46 minLvl=0
            setSkill(SKILL_CROSSBOWS);      // 226 minLvl=0
            setSkill(SKILL_AXES);           // 44 minLvl=0
            setSkill(SKILL_SWORDS);         // 43 minLvl=0
            setSkill(SKILL_2H_SWORDS);      // 55 minLvl=0
            setSkill(SKILL_2H_AXES);        // 172 minLvl=0
            setSkill(SKILL_DAGGERS);        // 173 minLvl=0
            setSkill(SKILL_STAVES);         // 136 minLvl=0
            setSkill(SKILL_THROWN);         // 176 minLvl=0
            setSkill(SKILL_POLEARMS);       // 229 minLvl=20
            setSkill(SKILL_FIST_WEAPONS);   // 473 minLvl=0
            break;
        }

        case CLASS_ROGUE: {
            // Armor: Leather, Cloth
            setSkill(SKILL_LEATHER);        // 414 minLvl=0
            setSkill(SKILL_CLOTH);          // 415 minLvl=0
            // Weapons: Daggers, Swords, Axes, Maces, Bows, Guns, Crossbows, Thrown, Fist
            setSkill(SKILL_DAGGERS);        // 173 minLvl=0
            setSkill(SKILL_SWORDS);         // 43 minLvl=0
            setSkill(SKILL_AXES);           // 44 minLvl=0
            setSkill(SKILL_MACES);          // 54 minLvl=0
            setSkill(SKILL_BOWS);           // 45 minLvl=0
            setSkill(SKILL_GUNS);           // 46 minLvl=0
            setSkill(SKILL_CROSSBOWS);      // 226 minLvl=0
            setSkill(SKILL_THROWN);         // 176 minLvl=0
            setSkill(SKILL_FIST_WEAPONS);   // 473 minLvl=0
            break;
        }

        case CLASS_PRIEST: {
            // Armor: Cloth
            setSkill(SKILL_CLOTH);          // 415 minLvl=0
            // Weapons: Maces, Staves, Daggers, Wands
            setSkill(SKILL_MACES);          // 54 minLvl=0
            setSkill(SKILL_STAVES);         // 136 minLvl=0
            setSkill(SKILL_DAGGERS);        // 173 minLvl=0
            setSkill(SKILL_WANDS);          // 228 minLvl=0
            break;
        }

        case CLASS_SHAMAN: {
            // Armor: Mail(40), Leather, Cloth
            setSkill(SKILL_MAIL);           // 413 minLvl=40
            setSkill(SKILL_LEATHER);        // 414 minLvl=0
            setSkill(SKILL_CLOTH);          // 415 minLvl=0
            // Weapons: Axes, Maces, Staves, 2H Maces, 2H Axes, Daggers, Shield, Fist
            setSkill(SKILL_AXES);           // 44 minLvl=0
            setSkill(SKILL_MACES);          // 54 minLvl=0
            setSkill(SKILL_STAVES);         // 136 minLvl=0
            setSkill(SKILL_2H_MACES);       // 160 minLvl=0
            setSkill(SKILL_2H_AXES);        // 172 minLvl=0
            setSkill(SKILL_DAGGERS);        // 173 minLvl=0
            setSkill(SKILL_SHIELD);         // 433 minLvl=0
            setSkill(SKILL_FIST_WEAPONS);   // 473 minLvl=0
            break;
        }

        case CLASS_MAGE: {
            // Armor: Cloth
            setSkill(SKILL_CLOTH);          // 415 minLvl=0
            // Weapons: Swords, Daggers, Staves, Wands
            setSkill(SKILL_SWORDS);         // 43 minLvl=0
            setSkill(SKILL_DAGGERS);        // 173 minLvl=0
            setSkill(SKILL_STAVES);         // 136 minLvl=0
            setSkill(SKILL_WANDS);          // 228 minLvl=0
            break;
        }

        case CLASS_WARLOCK: {
            // Armor: Cloth
            setSkill(SKILL_CLOTH);          // 415 minLvl=0
            // Weapons: Swords, Daggers, Staves, Wands
            setSkill(SKILL_SWORDS);         // 43 minLvl=0
            setSkill(SKILL_DAGGERS);        // 173 minLvl=0
            setSkill(SKILL_STAVES);         // 136 minLvl=0
            setSkill(SKILL_WANDS);          // 228 minLvl=0
            break;
        }

        case CLASS_DRUID: {
            // Armor: Leather, Cloth (NO mail, NO plate!)
            setSkill(SKILL_LEATHER);        // 414 minLvl=0
            setSkill(SKILL_CLOTH);          // 415 minLvl=0
            // Weapons: Maces, Staves, 2H Maces, Daggers, Polearms(20), Fist
            setSkill(SKILL_MACES);          // 54 minLvl=0
            setSkill(SKILL_STAVES);         // 136 minLvl=0
            setSkill(SKILL_2H_MACES);       // 160 minLvl=0
            setSkill(SKILL_DAGGERS);        // 173 minLvl=0
            setSkill(SKILL_POLEARMS);       // 229 minLvl=20
            setSkill(SKILL_FIST_WEAPONS);   // 473 minLvl=0
            break;
        }

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

// Map an item_template InventoryType to the equipment slot it fills (-1 = not
// gear). Two-hand and one-hand weapons both go to MAINHAND; the engine's
// CanEquipItem rejects the conflicting combo, so only the best per slot is kept.
static int PlayerbotInvTypeToSlot(uint32 invType)
{
    switch (invType)
    {
        case INVTYPE_HEAD:      return EQUIPMENT_SLOT_HEAD;       // 0
        case INVTYPE_NECK:      return EQUIPMENT_SLOT_NECK;       // 1
        case INVTYPE_SHOULDERS: return EQUIPMENT_SLOT_SHOULDERS;  // 2
        case INVTYPE_BODY:      return EQUIPMENT_SLOT_BODY;       // 3
        case INVTYPE_CHEST:     return EQUIPMENT_SLOT_CHEST;      // 4
        case INVTYPE_WAIST:     return EQUIPMENT_SLOT_WAIST;      // 5
        case INVTYPE_LEGS:      return EQUIPMENT_SLOT_LEGS;       // 6
        case INVTYPE_FEET:      return EQUIPMENT_SLOT_FEET;       // 7
        case INVTYPE_WRISTS:    return EQUIPMENT_SLOT_WRISTS;     // 8
        case INVTYPE_HANDS:     return EQUIPMENT_SLOT_HANDS;      // 9
        case INVTYPE_FINGER:    return EQUIPMENT_SLOT_FINGER1;    // 10
        case INVTYPE_TRINKET:   return EQUIPMENT_SLOT_TRINKET1;   // 12
        case INVTYPE_CLOAK:     return EQUIPMENT_SLOT_BACK;       // 14
        case INVTYPE_WEAPON:    return EQUIPMENT_SLOT_MAINHAND;   // 15
        case INVTYPE_2HWEAPON:  return EQUIPMENT_SLOT_MAINHAND;   // 15
        case INVTYPE_SHIELD:    return EQUIPMENT_SLOT_OFFHAND;    // 16
        case INVTYPE_RANGED:    return EQUIPMENT_SLOT_RANGED;     // 17
        case INVTYPE_TABARD:    return EQUIPMENT_SLOT_TABARD;     // 18
        default:                return -1;
    }
}

// R5: give each bot a FULL level-appropriate gear set at creation, generated
// from item_template (AC pattern: the engine's own item rules are authoritative,
// no curated table). A bot created at level N with no gear cannot fight mobs of
// its level (melee classes deal 1 dmg/hit, no armor) and dies — so this is a
// prerequisite for the grind/loot/gold economy to work at all.
//
// Per slot we keep the top-N candidates by score (ItemLevel, then Quality) among
// items that: the class may use (AllowableClass bit), RequiredLevel in
// [max(1, level-20), level], ItemLevel <= level+10 (so low bots don't get
// over-leveled junk), and no skill/city-rank/reputation gates. At equip time we
// try candidates best-first; if the engine rejects one (class/proficiency/
// two-hand conflict) we fall back to the next, so the bot always gets the best
// item it can actually use. The class starter junk (granted in Player::Create)
// is destroyed before equipping so our item always takes the slot.
void PlayerbotFactory::ApplyGear(Player* player, uint8 level, uint8 class_)
{
    if (!player)
        return;

    uint32 classBit = 1u << (class_ - 1);   // GetClassMask() convention = 1 << (class-1)
    uint32 minReq = (level > 20) ? (level - 20) : 1;
    uint32 maxILvl = (uint32)level + 10;    // avoid over-leveled items

    static const int MAXCAND = 16;
    struct Cand { uint32 entry; uint32 score; };
    Cand cands[EQUIPMENT_SLOT_END][MAXCAND];
    int candCount[EQUIPMENT_SLOT_END] = {};

    for (auto const& pair : sObjectMgr.GetItemPrototypeMap())
    {
        ItemPrototype const& proto = pair.second;
        if ((proto.AllowableClass & classBit) == 0)   continue;
        if (proto.RequiredLevel > level)             continue;
        if (proto.RequiredLevel < minReq)            continue;
        if (proto.ItemLevel > maxILvl)               continue;
        if (proto.RequiredSkill || proto.RequiredCityRank ||
            proto.RequiredReputationFaction)        continue;
        int slot = PlayerbotInvTypeToSlot(proto.InventoryType);
        if (slot < 0 || slot >= EQUIPMENT_SLOT_END)  continue;

        uint32 score = proto.ItemLevel * 10 + proto.Quality;
        int& cnt = candCount[slot];
        if (cnt < MAXCAND)
        {
            cands[slot][cnt] = { proto.ItemId, score };
            ++cnt;
        }
        else
        {
            int worst = 0;
            for (int i = 1; i < MAXCAND; ++i)
                if (cands[slot][i].score < cands[slot][worst].score) worst = i;
            if (score > cands[slot][worst].score)
                cands[slot][worst] = { proto.ItemId, score };
        }
    }

    int equipped = 0;
    for (int slot = 0; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        if (candCount[slot] == 0)
            continue;
        Cand* arr = cands[slot];
        // Sort this slot's candidates best-first (insertion sort, <=16 items).
        for (int i = 0; i < candCount[slot]; ++i)
            for (int j = i + 1; j < candCount[slot]; ++j)
                if (arr[j].score > arr[i].score) { Cand t = arr[i]; arr[i] = arr[j]; arr[j] = t; }

        bool done = false;
        for (int i = 0; i < candCount[slot] && !done; ++i)
        {
            ItemPrototype const* proto = sObjectMgr.GetItemPrototype(arr[i].entry);
            if (!proto)
                continue;
            uint16 dest = 0;
            // swap=true: the starter junk occupies the slot; FindEquipSlot only
            // accepts an occupied slot when swap=true. If the engine rejects this
            // candidate (class/proficiency/two-hand conflict) we try the next-best.
            if (player->CanEquipItem((uint8)slot, dest, proto, nullptr, true) != EQUIP_ERR_OK)
                continue;
            // Destroy the starter junk so our item always takes the slot.
            if (Item* existing = player->GetItemByPos(INVENTORY_SLOT_BAG_0, (uint8)slot))
                player->DestroyItem(INVENTORY_SLOT_BAG_0, (uint8)slot, false);
            Item* item = Item::CreateItem(arr[i].entry, 1, player);
            if (!item)
                continue;
            player->EquipItem((uint8)slot, item, true);
            done = true;
        }
        if (done)
            ++equipped;
    }

    LOG_DEBUG("playerbots", "[FACTORY] Applied generated gear (level %u class %u): %d items", level, class_, equipped);
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
