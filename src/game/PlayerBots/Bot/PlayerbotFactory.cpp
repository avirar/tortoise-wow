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

std::vector<std::pair<uint8, uint8>> PlayerbotFactory::s_validRaceClass;

void PlayerbotFactory::LoadValidRaceClassCombinations()
{
    if (!s_validRaceClass.empty())
        return; // already loaded

    // Use sObjectMgr.GetPlayerInfo() to check valid race/class combinations
    // This matches what Player::Create() uses internally
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

void PlayerbotFactory::GenerateBots(uint32 count, std::string const& accountPrefix)
{
    if (count == 0)
        return;

    CleanupOldBots(accountPrefix);

    // Load valid race/class combinations from playercreateinfo
    LoadValidRaceClassCombinations();
    if (s_validRaceClass.empty())
    {
        sLog.outError("Playerbot Factory: No valid race/class combinations found");
        return;
    }

    LOG_DEBUG("playerbots", ">> Playerbot Factory: Generating %u bot accounts and characters...", count);

    uint32 created = 0;
    for (uint32 i = 0; i < count; ++i)
    {
        LOG_DEBUG("playerbots", "[FACTORY] Starting bot #%u of %u...", i, count);
        uint32 accountId = CreateBotAccount(i, accountPrefix);
        if (!accountId)
        {
            sLog.outError("Playerbot Factory: Failed to create account #%u, skipping", i);
            continue;
        }
        LOG_DEBUG("playerbots", "[FACTORY] Account #%u created (id=%u)", i, accountId);

        LOG_DEBUG("playerbots", "[FACTORY] Creating character for account %u...", accountId);
        uint32 charGuid = CreateBotCharacter(accountId);
        if (!charGuid)
        {
            sLog.outError("Playerbot Factory: Failed to create character for account %u", accountId);
            continue;
        }
        LOG_DEBUG("playerbots", "[FACTORY] Character created (guid=%u)", charGuid);

        LOG_DEBUG("playerbots", "[FACTORY] Registering bot in playerbot table...");
        RegisterInPlayerbotTable(charGuid, 100, "PlayerBotAI");
        ++created;
        LOG_DEBUG("playerbots", "[FACTORY] Bot #%u registered, total=%u", i, created);

        if (created % 5 == 0)
            LOG_DEBUG("playerbots", ">> Playerbot Factory: %u/%u bots created", created, count);
    }

    LOG_DEBUG("playerbots", ">> Playerbot Factory: Successfully created %u/%u bots", created, count);
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
    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: generating GUID");
    uint32 guid = sObjectMgr.GeneratePlayerLowGuid();
    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: GUID=%u", guid);

    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: generating name");
    std::string name = GenerateName();
    if (name.empty())
    {
        sLog.outError("Playerbot Factory: Could not generate unique name");
        return 0;
    }
    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: name='%s'", name.c_str());

    // Pick random valid race/class combination
    uint32 comboIdx = urand(0, (uint32)s_validRaceClass.size() - 1);
    uint8 race = s_validRaceClass[comboIdx].first;
    uint8 class_ = s_validRaceClass[comboIdx].second;
    uint8 gender = urand(0, 1) ? GENDER_MALE : GENDER_FEMALE;

    // Get race/class names for logging
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(class_);
    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: race=%u (%s) class=%u (%s) gender=%u",
        race, rEntry ? rEntry->name[0] : "?", class_, cEntry ? cEntry->name[0] : "?", gender);

    uint8 skin = urand(0, 5);
    uint8 face = urand(0, 5);
    uint8 hairStyle = urand(0, 5);
    uint8 hairColor = urand(0, 5);
    uint8 facialHair = urand(0, 5);

    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: creating WorldSession");
    WorldSession* sess = new WorldSession(accountId, nullptr, SEC_PLAYER, 0, LOCALE_enUS, "<FACTORY>", 0);
    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: creating Player");
    Player* newChar = new Player(sess);
    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: calling Player::Create");
    if (!newChar->Create(guid, name, race, class_, gender, skin, face, hairStyle, hairColor, facialHair))
    {
        sLog.outError("Playerbot Factory: Player::Create failed for guid %u", guid);
        delete newChar;
        delete sess;
        return 0;
    }
    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: Player::Create done");

    newChar->SetCinematic(1);

    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: calling SaveToDB");
    if (!newChar->SaveToDB(true, false))
    {
        sLog.outError("Playerbot Factory: SaveToDB failed for guid %u", guid);
        delete newChar;
        delete sess;
        return 0;
    }
    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: SaveToDB done");

    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: deleting Player/Session");
    delete newChar;
    delete sess;
    LOG_DEBUG("playerbots", "[FACTORY] CreateBotCharacter: deleting done, loading cache");

    sObjectMgr.LoadPlayerCacheData(guid);

    LOG_DEBUG("playerbots", "Playerbot Factory: Created character '%s' (guid=%u, account=%u)", name.c_str(), guid, accountId);
    return guid;
}

std::string PlayerbotFactory::GenerateName()
{
    static const char* startSyl[] = {
        "Ar", "Bel", "Cor", "Dal", "El", "Fal", "Gor", "Hal", "Ir", "Jor",
        "Kael", "Lor", "Mor", "Nor", "Or", "Par", "Quel", "Ran", "Sil", "Tor",
        "Ul", "Var", "Wyn", "Xan", "Yor", "Zel"
    };
    static const uint32 startCount = sizeof(startSyl) / sizeof(startSyl[0]);

    static const char* endSyl[] = {
        "an", "ar", "as", "dor", "drin", "eth", "gar", "il", "ion", "is",
        "mar", "on", "or", "ric", "ros", "thas", "vin", "wyn", "yk", "us"
    };
    static const uint32 endCount = sizeof(endSyl) / sizeof(endSyl[0]);

    for (uint32 attempt = 0; attempt < 50; ++attempt)
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

void PlayerbotFactory::RegisterInPlayerbotTable(uint32 charGuid, uint32 chance, std::string const& aiName)
{
    CharacterDatabase.PExecute("INSERT INTO playerbot (char_guid, chance, ai) VALUES (%u, %u, '%s')",
        charGuid, chance, aiName.c_str());
}

void PlayerbotFactory::CleanupOldBots(std::string const& prefix)
{
    CharacterDatabase.PExecute("DELETE FROM playerbot");

    QueryResult* result = LoginDatabase.PQuery("SELECT id FROM account WHERE username LIKE '%s%%'",
        prefix.c_str());
    if (!result)
    {
        LOG_DEBUG("playerbots", "Playerbot Factory: No old factory accounts to clean up");
        return;
    }

    std::vector<uint32> accountIds;
    do
    {
        Field* fields = result->Fetch();
        accountIds.push_back(fields[0].GetUInt32());
    } while (result->NextRow());
    delete result;

    QueryResult* oldResult = LoginDatabase.PQuery("SELECT id FROM account WHERE username = 'BOT' OR username = 'bot'");
    if (oldResult)
    {
        do
        {
            Field* fields = oldResult->Fetch();
            accountIds.push_back(fields[0].GetUInt32());
        } while (oldResult->NextRow());
        delete oldResult;
    }

    for (uint32 accId : accountIds)
    {
        sAccountMgr.DeleteAccount(accId);
    }

    sAccountMgr.LoadAccountNames();

    LOG_DEBUG("playerbots", "Playerbot Factory: Cleaned up %u old bot accounts", (uint32)accountIds.size());
}
