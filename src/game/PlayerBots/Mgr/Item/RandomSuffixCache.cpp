/*
 * PlayerBots/Mgr/Item/RandomSuffixCache — see header for the vanilla
 * suffix model + why chance-weighted average is the right score.
 */
#include "RandomSuffixCache.h"

#include "Database/DatabaseEnv.h"
#include "Logging.h"
#include "Log.h"

namespace
{
    std::unordered_map<uint32, std::vector<SuffixOption>> s_options;
    bool s_loaded = false;
}

void RandomSuffixCache::EnsureLoaded()
{
    if (s_loaded)
        return;

    QueryResult* result = WorldDatabase.Query("SELECT entry, ench, chance FROM item_enchantment_template");
    uint32 rowCount = 0;
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            uint32 entry = f[0].GetUInt32();
            uint32 suffixId = f[1].GetUInt32();
            float chance = f[2].GetFloat();
            ++rowCount;

            // Same filter as core LoadRandomEnchantmentsTable
            if (chance > 0.000001f && chance <= 100.0f)
                s_options[entry].push_back(SuffixOption{suffixId, chance});
        } while (result->NextRow());
        delete result;
    }
    else
        sLog.outErrorDb("playerbots: item_enchantment_template query failed (green-item suffix scoring degraded)");

    s_loaded = true;
    sLog.outInfo("playerbots: suffix options loaded: %u green-item classes (%u rows)",
        uint32(s_options.size()), rowCount);
}

const std::vector<SuffixOption>* RandomSuffixCache::GetOptions(uint32 entry)
{
    EnsureLoaded();
    auto it = s_options.find(entry);
    return it != s_options.end() ? &it->second : nullptr;
}

bool RandomSuffixCache::IsLoaded()
{
    EnsureLoaded();
    return s_loaded;
}
