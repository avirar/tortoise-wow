/*
 * QuestValues — R7 L3: AI-facing quest DATA values (AC mod-playerbots
 * QuestValues.h port; names + checkInterval parity). Exposes the
 * PlayerQuestMgr startup cache (takers / objective POIs / giver entries)
 * to the AI layer, so the Rpg DO_QUEST state (L5) + quest actions (L4) can
 * walk to a real quest objective / taker / giver.
 *
 * AC parity: FreeQuestLogSlotValue (interval 2), ActiveQuestGivers/Takers/
 * ObjectivesValue (interval 5 -> 5000ms in this core's CalculatedValue).
 * The vector values return nearest-first position lists (QuestDest). The
 * givers value does a live nearby scan + PrepareQuestMenu (gated by
 * questEnabled, the expensive path); takers/objectives are cheap cache
 * lookups (always computed).
 */
#ifndef PLAYERBOT_QUEST_VALUES_H
#define PLAYERBOT_QUEST_VALUES_H

#include "Value/Value.h"
#include "PlayerQuestMgr.h"

#include <vector>

class PlayerBotAI;

// Free quest log slots (0..MAX_QUEST_LOG_SIZE).
class FreeQuestLogSlotValue : public Uint8CalculatedValue
{
public:
    FreeQuestLogSlotValue(PlayerBotAI* botAI)
        : Uint8CalculatedValue(botAI, "free quest log slots", 2) {}

    uint8 Calculate() override;
};

// Nearby live givers offering a WorthAccepting quest (questAcceptRadius), nearest-first.
class ActiveQuestGiversValue : public CalculatedValue<std::vector<PlayerQuestMgr::QuestDest> >
{
public:
    ActiveQuestGiversValue(PlayerBotAI* botAI)
        : CalculatedValue(botAI, "active quest givers", 5) {}

    std::vector<PlayerQuestMgr::QuestDest> Calculate() override;
};

// Taker POIs for the bot's complete (not yet rewarded) quests, nearest-first.
class ActiveQuestTakersValue : public CalculatedValue<std::vector<PlayerQuestMgr::QuestDest> >
{
public:
    ActiveQuestTakersValue(PlayerBotAI* botAI)
        : CalculatedValue(botAI, "active quest takers", 5) {}

    std::vector<PlayerQuestMgr::QuestDest> Calculate() override;
};

// Objective POIs for the bot's incomplete quests (kill objectives to do), nearest-first.
class ActiveQuestObjectivesValue : public CalculatedValue<std::vector<PlayerQuestMgr::QuestDest> >
{
public:
    ActiveQuestObjectivesValue(PlayerBotAI* botAI)
        : CalculatedValue(botAI, "active quest objectives", 5) {}

    std::vector<PlayerQuestMgr::QuestDest> Calculate() override;
};

#endif
