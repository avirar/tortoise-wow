/*
 * RpgBaseAction — R7 L2. See header for the port mapping.
 */

#include "RpgBaseAction.h"
#include "PlayerBotAI.h"
#include "PlayerbotAIConfig.h"
#include "PlayerGrindMgr.h"
#include "PlayerQuestMgr.h"
#include "UnitDefines.h"
#include "AiObjectContext.h"   // AI_VALUE macro needs the complete context type
#include "Logging.h"
#include "QuestDef.h"
#include "Maps/Map.h"
#include "Maps/CellImpl.h"
#include "Maps/GridNotifiers.h"
#include "Maps/GridMap.h"
#include "Mgr/Item/StatsWeightCalculator.h"
#include <string>
#include <cmath>
#include <vector>

// L4/L5: cell-scan visitor for nearby quest-givers (mirrors GiverScannerVisitor
// in PlayerQuestMgr.cpp). Collects live creatures whose entry is in the giver set.
struct RpgGiverScanner
{
    Unit const* me;
    std::set<uint32> const* giverEntries;
    float range;
    std::vector<Creature*> candidates;

    RpgGiverScanner() : me(nullptr), giverEntries(nullptr), range(0) {}
    void Init(Unit const* m, std::set<uint32> const* ge, float r)
    {
        me = m; giverEntries = ge; range = r; candidates.clear();
    }

    void CheckCreature(Creature* cre)
    {
        if (!giverEntries || !giverEntries->count(cre->GetEntry()) || !cre->IsAlive())
            return;
        if (me->GetDistance(cre) <= range)
            candidates.push_back(cre);
    }

    void Visit(PlayerMapType &) {}
    void Visit(CreatureMapType &m)
    {
        for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            CheckCreature(itr->getSource());
    }
    template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    template<class NOT_INTERESTED> void Visit(NOT_INTERESTED &) {}
};

bool RpgBaseAction::CheckRpgStatusAvailable(PlayerRpgStatus status)
{
    switch (status)
    {
        case RPG_IDLE:
        case RPG_REST:
            return true;
        case RPG_WANDER_RANDOM:
            // AC :1239 — only wander when there's a nearby grind target to mill around.
            return AI_VALUE(Unit*, "grind target") != nullptr;
        case RPG_GO_GRIND:
            // AC :1245 — only go-grind when a grind spot is actually available.
            return SelectRandomGrindPos();
        case RPG_DO_QUEST:
            // L4/L5: only do a quest when the bot has one with a reachable POI.
            { uint32 qid; return SelectDoQuestQuest(qid); }
        default:
            return false;
    }
}

bool RpgBaseAction::SelectRandomGrindPos()
{
    PlayerGrindMgr::Spot spot;
    return PlayerGrindMgr::SelectRandomGrindPos(bot, spot);
}

bool RpgBaseAction::RandomChangeStatus()
{
    // Build the candidate set: weight > 0 AND available (AC NewRpgBaseAction.cpp:1060-1080).
    std::vector<PlayerRpgStatus> available;
    float probSum = 0.0f;
    for (int32 i = 0; i < RPG_MAX_STATUS; ++i)
    {
        PlayerRpgStatus status = static_cast<PlayerRpgStatus>(i);
        if (sPlayerbotAIConfig.rpgStatusProbWeight[i] > 0.0f && CheckRpgStatusAvailable(status))
        {
            available.push_back(status);
            probSum += sPlayerbotAIConfig.rpgStatusProbWeight[i];
        }
    }

    // Safety: default to rest if nothing is available (AC :1085-1091).
    if (available.empty() || probSum <= 0.0f)
    {
        botAI->rpgInfo.ChangeToRest();
        if (bot && bot->IsAlive())
            bot->SetStandState(UNIT_STAND_STATE_SIT);
        return true;
    }

    // Weighted random pick. AC does `urand(1, probSum)` (float->uint truncation, a
    // latent AC bug); we do a proper float roll instead.
    float roll = (float)irand(0, 99999) / 99999.0f * probSum;
    float accumulate = 0.0f;
    PlayerRpgStatus chosen = RPG_MAX_STATUS;
    for (std::vector<PlayerRpgStatus>::iterator i = available.begin(); i != available.end(); ++i)
    {
        accumulate += sPlayerbotAIConfig.rpgStatusProbWeight[*i];
        if (accumulate >= roll)
        {
            chosen = *i;
            break;
        }
    }
    if (chosen == RPG_MAX_STATUS)
        chosen = available.back();

    // Low-noise diagnostic: log each status pick (naturally low-frequency —
    // at most once per status window), so the state machine's cycling is
    // visible in info.log without flooding it.
    if (bot)
        sLog.outInfo("playerbots: rpg %s -> %s", bot->GetName(), PlayerRpgInfo::StatusToString(chosen));

    switch (chosen)
    {
        case RPG_WANDER_RANDOM:
            botAI->rpgInfo.ChangeToWanderRandom();
            return true;
        case RPG_GO_GRIND:
            // AC :1144 — pick the grind spot now (data setup), fail soft if none.
            {
                PlayerGrindMgr::Spot spot;
                if (!PlayerGrindMgr::SelectRandomGrindPos(bot, spot))
                    return false;
                // Same-map only (grind spots are cached per map; MoveFarTo never
                // crosses maps). The L1 ChangeToGoGrind stores x/y/z (no map id).
                botAI->rpgInfo.ChangeToGoGrind(spot.x, spot.y, spot.z);
                return true;
            }
        case RPG_REST:
            botAI->rpgInfo.ChangeToRest();
            if (bot && bot->IsAlive())
                bot->SetStandState(UNIT_STAND_STATE_SIT);
            return true;
        case RPG_IDLE:
            botAI->rpgInfo.ChangeToIdle();
            return true;
        case RPG_DO_QUEST:
            // L4/L5: pick an active quest with a reachable POI (AC DO_QUEST case).
            { uint32 qid; if (!SelectDoQuestQuest(qid)) return false;
              botAI->rpgInfo.ChangeToDoQuest(qid, -1); return true; }
        default:
            botAI->rpgInfo.ChangeToRest();
            if (bot && bot->IsAlive())
                bot->SetStandState(UNIT_STAND_STATE_SIT);
            return true;
    }
}

// ---- L4/L5: quest helpers (AC NewRpgBaseAction quest layer, rewritten for our
// chassis + the L3 quest cache). ----

bool RpgBaseAction::SearchQuestGiverAndAcceptOrReward()
{
    // AC NewRpgBaseAction::SearchQuestGiverAndAcceptOrReward. Overworld + alive
    // + not in combat (don't accept/turn-in mid-fight).
    if (!bot || !bot->IsAlive() || bot->IsInCombat())
        return false;
    Map* map = bot->GetMap();
    if (!map || map->IsDungeon())
        return false;

    WorldObject* giver = nullptr;
    if (!ChooseGiverToInteract(giver, (float)sPlayerbotAIConfig.questAcceptRadius))
        return false;

    // Pacing (AC ForceToWait(5000)): after an accept/turn-in, don't re-interact for
    // 5s (spaces out rapid-fire menu interactions; the menu is rebuilt next tick).
    const uint32 now = getMSTime();
    if (now - botAI->rpgInfo.lastQuestInteract < 5000)
        return MoveWorldObjectTo(giver->GetObjectGuid());

    if (bot->CanInteractWithQuestGiver(giver))
    {
        if (InteractWithGiverForQuest(giver))
        {
            botAI->rpgInfo.lastQuestInteract = now;  // pace the next interaction
            return true;  // consumed the tick (menu is rebuilt next tick)
        }
    }
    // Walk toward the giver (AC MoveWorldObjectTo).
    return MoveWorldObjectTo(giver->GetObjectGuid());
}

bool RpgBaseAction::HasQuestToAcceptOrReward(WorldObject* object)
{
    // AC NewRpgBaseAction::HasQuestToAcceptOrReward. PrepareQuestMenu + scan:
    // any COMPLETE+CanReward (turn-in) or NONE+WorthAccepting (accept).
    bot->PrepareQuestMenu(object->GetObjectGuid());
    if (!bot->PlayerTalkClass)
        return false;
    QuestMenu& menu = bot->PlayerTalkClass->GetQuestMenu();
    if (menu.Empty())
        return false;
    for (uint16 idx = 0; idx < menu.MenuItemCount(); ++idx)
    {
        Quest const* quest = sObjectMgr.GetQuestTemplate(menu.GetItem(idx).m_qId);
        if (!quest)
            continue;
        if (bot->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_COMPLETE &&
            bot->CanRewardQuest(quest, false))
            return true;
    }
    for (uint16 idx = 0; idx < menu.MenuItemCount(); ++idx)
    {
        Quest const* quest = sObjectMgr.GetQuestTemplate(menu.GetItem(idx).m_qId);
        if (!quest)
            continue;
        if (bot->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_NONE &&
            bot->CanTakeQuest(quest, false) && bot->CanAddQuest(quest, false) &&
            PlayerQuestMgr::IsWorthAccepting(bot, quest))
            return true;
    }
    return false;
}

bool RpgBaseAction::ChooseGiverToInteract(WorldObject*& outObject, float distLimit)
{
    // AC ChooseNpcOrGameObjectToInteract (quests only). Nearest live giver with a
    // quest to accept/reward (cell scan over the giver entries).
    outObject = nullptr;
    Map* map = bot->GetMap();
    if (!map)
        return false;
    RpgGiverScanner scanner;
    scanner.Init(bot, &PlayerQuestMgr::GetGiverEntries(), distLimit);
    CellPair p(MaNGOS::ComputeCellPair(bot->GetPositionX(), bot->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();
    TypeContainerVisitor<RpgGiverScanner, WorldTypeMapContainer> world_vis(scanner);
    TypeContainerVisitor<RpgGiverScanner, GridTypeMapContainer> grid_vis(scanner);
    cell.Visit(p, world_vis, *map, *bot, distLimit);
    cell.Visit(p, grid_vis, *map, *bot, distLimit);

    WorldObject* nearest = nullptr;
    for (std::vector<Creature*>::const_iterator i = scanner.candidates.begin(); i != scanner.candidates.end(); ++i)
    {
        Creature* c = *i;
        if (!c || !c->IsAlive())
            continue;
        if (!HasQuestToAcceptOrReward(c))
            continue;
        if (!nearest || bot->GetDistance(nearest, SizeFactor::None) > bot->GetDistance(c, SizeFactor::None))
            nearest = c;
    }
    if (nearest)
    {
        outObject = nearest;
        return true;
    }
    return false;
}

bool RpgBaseAction::InteractWithGiverForQuest(WorldObject* giver)
{
    Creature* c = giver->ToCreature();
    if (!c || !c->IsAlive())
        return false;
    bot->PrepareQuestMenu(giver->GetObjectGuid());
    if (!bot->PlayerTalkClass)
        return false;
    QuestMenu& menu = bot->PlayerTalkClass->GetQuestMenu();
    if (menu.Empty())
        return false;
    // 1) Turn in any COMPLETE quest first (AC rewards before new accepts).
    for (uint16 idx = 0; idx < menu.MenuItemCount(); ++idx)
    {
        Quest const* quest = sObjectMgr.GetQuestTemplate(menu.GetItem(idx).m_qId);
        if (!quest)
            continue;
        if (bot->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_COMPLETE &&
            !bot->GetQuestRewardStatus(quest->GetQuestId()) &&
            bot->CanRewardQuest(quest, false))
        {
            if (TurnInQuestAtGiver(c, quest))
                return true;
        }
    }
    // 2) Accept a new quest (NONE + WorthAccepting).
    for (uint16 idx = 0; idx < menu.MenuItemCount(); ++idx)
    {
        Quest const* quest = sObjectMgr.GetQuestTemplate(menu.GetItem(idx).m_qId);
        if (!quest)
            continue;
        if (bot->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_NONE &&
            PlayerQuestMgr::IsWorthAccepting(bot, quest))
        {
            if (AcceptQuestAtGiver(c, quest))
                return true;
        }
    }
    return false;
}

bool RpgBaseAction::AcceptQuestAtGiver(Creature* giver, Quest const* quest)
{
    uint32 questId = quest->GetQuestId();
    if (!bot->CanTakeQuest(quest, false) || !bot->CanAddQuest(quest, false))
        return false;
    bot->AddQuest(quest, giver);
    botAI->rpgLowPriorityQuest.erase(questId);  // fresh quest — clear any stale abandon
    sLog.outInfo("playerbots: %s accepted quest %u '%s' (lvl %u)",
        bot->GetName(), questId, quest->GetTitle().c_str(), (unsigned)bot->GetLevel());
    // AC: auto-complete quests that are done immediately (talk-to / item).
    if (bot->CanCompleteQuest(questId))
        bot->CompleteQuest(questId);
    return true;
}

bool RpgBaseAction::TurnInQuestAtGiver(Creature* giver, Quest const* quest)
{
    uint32 questId = quest->GetQuestId();
    // CompleteQuest marks COMPLETE (+ auto-rewards if QUEST_FLAGS_AUTO_REWARDED).
    if (bot->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
        bot->CompleteQuest(questId);
    if (bot->GetQuestRewardStatus(questId))
    {
        sLog.outInfo("playerbots: %s turned in quest %u '%s' (auto-reward)",
            bot->GetName(), questId, quest->GetTitle().c_str());
        return true;
    }
    uint32 bestIdx = BestRewardIndex(quest);
    if (bot->CanRewardQuest(quest, bestIdx, false))
    {
        bot->RewardQuest(quest, bestIdx, giver, false);
        sLog.outInfo("playerbots: %s turned in quest %u '%s' (reward choice %u)",
            bot->GetName(), questId, quest->GetTitle().c_str(), (unsigned)bestIdx);
    }
    botAI->rpgLowPriorityQuest.erase(questId);
    return true;
}

uint32 RpgBaseAction::BestRewardIndex(Quest const* quest)
{
    uint32 count = quest->GetRewChoiceItemsCount();
    if (count <= 1)
        return 0;
    uint32 bestIdx = 0;
    float bestScore = -1.0f;
    for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT && i < count; ++i)
    {
        uint32 itemId = quest->RewChoiceItemId[i];
        if (!itemId)
            continue;
        float score = StatsWeightCalculator(bot).CalculateItem(itemId);
        if (score > bestScore)
        {
            bestScore = score;
            bestIdx = i;
        }
    }
    return bestIdx;
}

bool RpgBaseAction::SelectDoQuestQuest(uint32& outQuestId)
{
    // AC DO_QUEST case: iterate the quest log, skip low-priority, pick a quest
    // with a reachable POI (objective if incomplete, taker if complete).
    std::vector<uint32> available;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;
        if (botAI->rpgLowPriorityQuest.count(questId))
            continue;
        Quest const* quest = sObjectMgr.GetQuestTemplate(questId);
        if (!quest)
            continue;
        QuestStatus status = bot->GetQuestStatus(questId);
        if (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE)
            continue;
        PlayerQuestMgr::QuestDest poi;
        bool ok = false;
        if (status == QUEST_STATUS_COMPLETE)
            ok = PlayerQuestMgr::GetQuestTakerPoi(bot, questId, poi);
        else
        {
            uint8 idx;
            ok = PlayerQuestMgr::GetQuestObjectivePoi(bot, questId, poi, idx);
        }
        if (ok)
            available.push_back(questId);
    }
    if (available.empty())
        return false;
    outQuestId = available[irand(0, (int)available.size() - 1)];
    return true;
}
