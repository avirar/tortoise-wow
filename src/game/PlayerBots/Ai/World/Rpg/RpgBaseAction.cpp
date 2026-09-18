/*
 * RpgBaseAction — R7 L2. See header for the port mapping.
 */

#include "RpgBaseAction.h"
#include "PlayerBotAI.h"
#include "PlayerbotAIConfig.h"
#include "PlayerGrindMgr.h"
#include "UnitDefines.h"
#include "AiObjectContext.h"   // AI_VALUE macro needs the complete context type
#include "Logging.h"
#include <string>

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
            // Deferred to L3+ (quest POI pipeline). Never auto-selected yet.
            return false;
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
        default:
            botAI->rpgInfo.ChangeToRest();
            if (bot && bot->IsAlive())
                bot->SetStandState(UNIT_STAND_STATE_SIT);
            return true;
    }
}
