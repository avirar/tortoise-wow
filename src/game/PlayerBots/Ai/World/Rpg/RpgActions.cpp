/*
 * Rpg actions — R7 L2. See header for the port mapping.
 */

#include "RpgActions.h"
#include "PlayerBotAI.h"
#include "PlayerbotAIConfig.h"
#include <cmath>

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
    // AC NewRpgGoGrindAction: also SearchQuestGiverAndAcceptOrReward() first (the
    // L3+ quest pipeline — deferred). Then WALK to the grind spot (same-map).
    // If the far path can't be laid, take a small nudge so the next tick's
    // MoveFarTo retries from a slightly different position (AC, verbatim).
    if (MoveFarTo(bot->GetMapId(), rpgInfo.grindX, rpgInfo.grindY, rpgInfo.grindZ))
        return true;
    return MoveRandomNear(10.0f);
}

bool RpgWanderRandomAction::Execute(Event event)
{
    // AC NewRpgWanderRandomAction: MoveRandomNear(0, ACTION_DEFAULT) — a random
    // short-distance wander around the current position.
    return MoveRandomNear();
}
