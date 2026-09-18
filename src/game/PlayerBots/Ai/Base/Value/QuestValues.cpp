/*
 * QuestValues — R7 L3: AI-facing quest data values (see header for design).
 * Thin wrappers over the PlayerQuestMgr startup cache + live scan.
 */
#include "QuestValues.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "PlayerQuestMgr.h"
#include "PlayerbotAIConfig.h"

uint8 FreeQuestLogSlotValue::Calculate()
{
    if (!bot)
        return 0;
    return PlayerQuestMgr::GetFreeQuestLogSlots(bot);
}

std::vector<PlayerQuestMgr::QuestDest> ActiveQuestGiversValue::Calculate()
{
    std::vector<PlayerQuestMgr::QuestDest> out;
    if (!bot)
        return out;
    // On-demand (lazy, 5s cache): live nearby scan + PrepareQuestMenu. AC's
    // ActiveQuestGiversValue is a plain value (not config-gated); the sweep
    // (ProcessBot) stays separately gated by questEnabled.
    PlayerQuestMgr::GetNearbyGivers(bot, out);
    return out;
}

std::vector<PlayerQuestMgr::QuestDest> ActiveQuestTakersValue::Calculate()
{
    std::vector<PlayerQuestMgr::QuestDest> out;
    if (!bot)
        return out;
    PlayerQuestMgr::GetActiveTakers(bot, out);
    return out;
}

std::vector<PlayerQuestMgr::QuestDest> ActiveQuestObjectivesValue::Calculate()
{
    std::vector<PlayerQuestMgr::QuestDest> out;
    if (!bot)
        return out;
    PlayerQuestMgr::GetActiveObjectives(bot, out);
    return out;
}
