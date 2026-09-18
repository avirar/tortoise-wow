/*
 * RpgBaseAction — R7 L2 base for the RPG state-machine actions.
 *
 * Port of AC mod-playerbots `NewRpgBaseAction` (azerothcore-wotlk
 * modules/mod-playerbots src/Ai/World/Rpg/Action/NewRpgBaseAction.h/.cpp).
 * AC's NewRpgBaseAction bundles far-movement helpers (MoveFarTo, MoveWorldObjectTo,
 * MoveRandomNear) + the state-machine helpers (RandomChangeStatus,
 * CheckRpgStatusAvailable, SelectRandomGrindPos/CampPos/Flight). In our chassis the
 * movement helpers live on `MovementAction` (L1, MovementActions.h) — so this base
 * just extends MovementAction and provides the state-machine helpers:
 *
 *   - RandomChangeStatus():  weighted-random pick of an AVAILABLE status (AC
 *     NewRpgBaseAction.cpp:1080-1225), incl. per-status data setup (GO_GRIND picks
 *     a grind spot, REST sits). AC's transition-matrix (ChangeStatus's big switch)
 *     is commented out in AC — it relies on this weighted-random, so we do too.
 *   - CheckRpgStatusAvailable(status): per-status gate (AC :1229-1283):
 *     WANDER needs a nearby grind target, GO_GRIND needs a grind spot, IDLE/REST
 *     always. DO_QUEST (L4/L5) needs an active quest with a reachable POI.
 *
 * R7 L4/L5: the quest helpers (AC NewRpgBaseAction::SearchQuestGiverAndAcceptOr
 * Reward / HasQuestToAcceptOrReward / ChooseNpcOrGameObjectToInteract + the
 * AcceptQuest/RewardQuest primitives) are added here so the GO_GRIND / WANDER /
 * DO_QUEST actions can pick up new quests + turn in completed ones while milling.
 *
 * Vanilla reductions (bot-master-plan.md R7, L2): GoCamp/WanderNpc/TravelFlight/
 * OutdoorPvP statuses omitted — GO_GRIND / WANDER_RANDOM / IDLE / REST / DO_QUEST
 * are auto-selected. Movement itself (walking) is the L1 MovementAction base.
 */
#ifndef _PLAYERBOT_RPG_BASE_ACTION_H
#define _PLAYERBOT_RPG_BASE_ACTION_H

#include "MovementActions.h"
#include "PlayerRpgInfo.h"

class Quest;
class WorldObject;
class Creature;

class RpgBaseAction : public MovementAction
{
public:
    explicit RpgBaseAction(PlayerBotAI* botAI) : MovementAction(botAI) {}

    // Weighted-random status transition (AC NewRpgBaseAction::RandomChangeStatus).
    // Picks from available (weight>0 && CheckRpgStatusAvailable) statuses and does
    // the per-status setup (grind-spot pick / sit). Returns true when it changed.
    bool RandomChangeStatus();

    // Per-status availability gate (AC NewRpgBaseAction::CheckRpgStatusAvailable).
    bool CheckRpgStatusAvailable(PlayerRpgStatus status);

    // AC NewRpgBaseAction::SearchQuestGiverAndAcceptOrReward — the do-quest
    // entry point. Finds a nearby quest-giver with a quest to accept/reward
    // (<=questAcceptRadius yd); if in interaction range, does the accept/turn-in
    // (CMSG replay) + a short wait; else walks to the giver. Returns true when it
    // consumed the tick (interacted or started walking to a giver).
    bool SearchQuestGiverAndAcceptOrReward();

protected:
    // Fill botAI->rpgInfo.data with a random grind spot (same map, level band,
    // <=2500yd) via PlayerGrindMgr. Returns true when a spot was found.
    bool SelectRandomGrindPos();

    // AC NewRpgBaseAction::HasQuestToAcceptOrReward — PrepareQuestMenu + scan:
    // any COMPLETE+CanReward (turn-in) or NONE+WorthAccepting (accept) quest.
    bool HasQuestToAcceptOrReward(WorldObject* object);
    // AC ChooseNpcOrGameObjectToInteract (quests only) — nearest nearby giver
    // (giver-entries cell scan) with a quest to accept/reward.
    bool ChooseGiverToInteract(WorldObject*& outObject, float distLimit);
    // Accept/turn-in primitives (direct Player calls; AC AddQuest/RewardQuest).
    bool InteractWithGiverForQuest(WorldObject* giver);
    bool AcceptQuestAtGiver(Creature* giver, Quest const* quest);
    bool TurnInQuestAtGiver(Creature* giver, Quest const* quest);
    uint32 BestRewardIndex(Quest const* quest);
    // Pick an active quest (in the log) with a reachable POI (AC DO_QUEST case).
    bool SelectDoQuestQuest(uint32& outQuestId);
};

#endif
