/*
 * Rpg actions — R7 L2. See header for the port mapping.
 */

#include "RpgActions.h"
#include "PlayerBotAI.h"
#include "PlayerbotAIConfig.h"
#include "PlayerQuestMgr.h"
#include "Logging.h"
#include "QuestDef.h"
#include <cmath>

// L4/L5 POI helpers (the Rpg DO_QUEST state machine tracks the current POI in
// rpgInfo; (0,0,0) = "no POI" sentinel, mirroring AC's WorldPosition()).
static bool RpgHasPoi(PlayerRpgInfo const& info)
{
    return info.poiMapId != 0 || info.poiX != 0.0f || info.poiY != 0.0f || info.poiZ != 0.0f;
}
static void RpgClearPoi(PlayerRpgInfo& info)
{
    info.poiMapId = 0; info.poiX = info.poiY = info.poiZ = 0.0f; info.lastReachPOI = 0;
}
static void RpgSetPoi(PlayerRpgInfo& info, PlayerQuestMgr::QuestDest const& poi)
{
    info.poiMapId = poi.map; info.poiX = poi.x; info.poiY = poi.y; info.poiZ = poi.z;
    info.lastReachPOI = 0;
}

bool RpgStatusUpdateAction::Execute(Event event)
{
    PlayerRpgInfo& rpgInfo = botAI->rpgInfo;
    PlayerRpgStatus status = rpgInfo.GetStatus();

    switch (status)
    {
        case RPG_IDLE:
            // AC :50 — from IDLE, pick a fresh status (weighted random).
            return RandomChangeStatus();

        case RPG_GO_GRIND:
            // AC :53 — once we've reached the grind spot, switch to WANDER (mill
            // around it while the base grind strategy kills the mobs that are there).
            {
                float dx = rpgInfo.grindX - bot->GetPositionX();
                float dy = rpgInfo.grindY - bot->GetPositionY();
                float dis = std::sqrt(dx * dx + dy * dy);
                if (dis < 10.0f)
                {
                    rpgInfo.ChangeToWanderRandom();
                    return true;
                }
                return false;  // still walking — the go-grind action moves us
            }

        case RPG_WANDER_RANDOM:
            if (!rpgInfo.HasStatusPersisted(sPlayerbotAIConfig.rpgWanderRandomStatusMaxDuration * 1000))
                return false;
            rpgInfo.ChangeToIdle();
            return true;

        case RPG_REST:
            if (!rpgInfo.HasStatusPersisted(sPlayerbotAIConfig.rpgRestStatusMaxDuration * 1000))
                return false;
            rpgInfo.ChangeToIdle();
            return true;

        case RPG_DO_QUEST:
            // Deferred (L3+). If somehow entered, bail to IDLE after its window.
            if (!rpgInfo.HasStatusPersisted(sPlayerbotAIConfig.rpgDoQuestStatusMaxDuration * 1000))
                return false;
            rpgInfo.ChangeToIdle();
            return true;

        default:
            return false;
    }
}

bool RpgGoGrindAction::Execute(Event event)
{
    PlayerRpgInfo& rpgInfo = botAI->rpgInfo;
    // AC NewRpgGoGrindAction: SearchQuestGiverAndAcceptOrReward() first — pick up
    // new quests / turn in completed ones while milling (L4/L5 quest pipeline).
    if (SearchQuestGiverAndAcceptOrReward())
        return true;
    // WALK to the grind spot (same-map). If the far path can't be laid, take a
    // small nudge so the next tick's MoveFarTo retries from a slightly different
    // position (AC, verbatim).
    if (MoveFarTo(bot->GetMapId(), rpgInfo.grindX, rpgInfo.grindY, rpgInfo.grindZ))
        return true;
    return MoveRandomNear(10.0f);
}

bool RpgWanderRandomAction::Execute(Event event)
{
    // AC NewRpgWanderRandomAction: SearchQuestGiverAndAcceptOrReward() first (the
    // bot may be milling around a town with a giver nearby), then a random
    // short-distance wander (MoveRandomNear(0, ACTION_DEFAULT)).
    if (SearchQuestGiverAndAcceptOrReward())
        return true;
    return MoveRandomNear();
}

bool RpgDoQuestAction::Execute(Event event)
{
    // AC NewRpgDoQuestAction: first, try to interact with a nearby giver (accept
    // a new quest / turn in a completed one).
    if (SearchQuestGiverAndAcceptOrReward())
        return true;

    PlayerRpgInfo& info = botAI->rpgInfo;
    if (info.questId == 0)
    {
        info.ChangeToIdle();
        return true;
    }
    QuestStatus status = bot->GetQuestStatus(info.questId);
    switch (status)
    {
        case QUEST_STATUS_INCOMPLETE:
            return DoIncompleteQuest();
        case QUEST_STATUS_COMPLETE:
            return DoCompletedQuest();
        default:
            // Quest no longer in the log (turned in / dropped) → back to IDLE.
            info.ChangeToIdle();
            return true;
    }
}

bool RpgDoQuestAction::DoIncompleteQuest()
{
    PlayerRpgInfo& info = botAI->rpgInfo;
    uint32 questId = info.questId;
    Quest const* quest = sObjectMgr.GetQuestTemplate(questId);
    if (!quest)
    {
        info.ChangeToIdle();
        return true;
    }
    QuestStatusMap& qmap = bot->getQuestStatusMap();
    QuestStatusMap::iterator it = qmap.find(questId);
    if (it == qmap.end())
    {
        info.ChangeToIdle();
        return true;
    }
    QuestStatusData const& qst = it->second;

    // If the current objective is now done, clear the POI (re-pick next tick).
    if (RpgHasPoi(info) && info.objectiveIdx >= 0 && info.objectiveIdx < QUEST_OBJECTIVES_COUNT)
    {
        int32 oi = info.objectiveIdx;
        if (qst.m_creatureOrGOcount[oi] >= quest->ReqCreatureOrGOCount[oi])
            RpgClearPoi(info);
    }

    // No POI → pick the nearest still-to-do kill objective (same map/zone/dist).
    if (!RpgHasPoi(info))
    {
        PlayerQuestMgr::QuestDest poi;
        uint8 objIdx;
        if (!PlayerQuestMgr::GetQuestObjectivePoi(bot, questId, poi, objIdx))
        {
            info.ChangeToIdle();  // can't find an objective POI → stop doing it
            return true;
        }
        RpgSetPoi(info, poi);
        info.objectiveIdx = (int32)objIdx;
    }

    // Far from the objective → WALK there (L1 MoveFarTo, pathfinding).
    float dx = info.poiX - bot->GetPositionX();
    float dy = info.poiY - bot->GetPositionY();
    float dis = std::sqrt(dx * dx + dy * dy);
    if (dis > 10.0f && !info.lastReachPOI)
    {
        if (MoveFarTo(info.poiMapId, info.poiX, info.poiY, info.poiZ))
            return true;
        return MoveRandomNear(10.0f);  // nudge so the next tick retries
    }

    // Near the objective. Mark reached (the base grind strategy does the kills).
    if (!info.lastReachPOI)
    {
        info.lastReachPOI = getMSTime();
        return true;
    }
    // Stayed at the POI > questNoProgressSeconds (AC poiStayTime 5min) with no
    // kill progress → abandon (low-priority; the bot won't re-pick it).
    if (getMSTime() - info.lastReachPOI >= (uint32)sPlayerbotAIConfig.questNoProgressSeconds * 1000)
    {
        int32 oi = info.objectiveIdx;
        bool hasProgression = (oi >= 0 && oi < QUEST_OBJECTIVES_COUNT &&
            qst.m_creatureOrGOcount[oi] > 0 && quest->ReqCreatureOrGOCount[oi] > 0);
        if (!hasProgression)
        {
            botAI->rpgLowPriorityQuest.insert(questId);
            sLog.outInfo("playerbots: %s abandoned quest %u '%s' (no kill progress at POI)",
                bot->GetName(), questId, quest->GetTitle().c_str());
            info.ChangeToIdle();
            return true;
        }
        // Progress made → clear the POI and re-pick (next objective / nearer POI).
        RpgClearPoi(info);
        return true;
    }
    // At the POI: a small wander while the grind strategy kills the mobs (AC
    // MoveRandomNear(8) — keep the bot actively placed near the objective).
    return MoveRandomNear(8.0f);
}

bool RpgDoQuestAction::DoCompletedQuest()
{
    PlayerRpgInfo& info = botAI->rpgInfo;
    uint32 questId = info.questId;
    // Walk to the quest ender (taker); the turn-in fires via
    // SearchQuestGiverAndAcceptOrReward (at the top of Execute) once the bot is
    // within interaction range.
    PlayerQuestMgr::QuestDest poi;
    if (!PlayerQuestMgr::GetQuestTakerPoi(bot, questId, poi))
    {
        info.ChangeToIdle();
        return true;
    }
    if (!RpgHasPoi(info))
    {
        RpgSetPoi(info, poi);
        info.objectiveIdx = -1;
    }

    float dx = info.poiX - bot->GetPositionX();
    float dy = info.poiY - bot->GetPositionY();
    float dis = std::sqrt(dx * dx + dy * dy);
    if (dis > 10.0f && !info.lastReachPOI)
    {
        if (MoveFarTo(info.poiMapId, info.poiX, info.poiY, info.poiZ))
            return true;
        return MoveRandomNear(10.0f);
    }
    if (!info.lastReachPOI)
    {
        info.lastReachPOI = getMSTime();
        return true;
    }
    // Reached the taker but the turn-in hasn't fired (e.g. the ender isn't a
    // scan-able giver, or the menu is empty) → after the no-progress window,
    // abandon rather than sit here indefinitely.
    if (getMSTime() - info.lastReachPOI >= (uint32)sPlayerbotAIConfig.questNoProgressSeconds * 1000)
    {
        botAI->rpgLowPriorityQuest.insert(questId);
        sLog.outInfo("playerbots: %s abandoned quest %u '%s' (no reward at taker)",
            bot->GetName(), questId, sObjectMgr.GetQuestTemplate(questId) ? sObjectMgr.GetQuestTemplate(questId)->GetTitle().c_str() : "?");
        info.ChangeToIdle();
        return true;
    }
    return false;  // wait for the turn-in to fire
}
