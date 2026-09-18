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
 *     always. DO_QUEST is deferred (L3+: needs the quest POI pipeline).
 *
 * Vanilla reductions (bot-master-plan.md R7, L2): GoCamp/WanderNpc/TravelFlight/
 * OutdoorPvP/DoQuest statuses omitted — only GO_GRIND / WANDER_RANDOM / IDLE /
 * REST are auto-selected. Movement itself (walking) is the L1 MovementAction base.
 */
#ifndef _PLAYERBOT_RPG_BASE_ACTION_H
#define _PLAYERBOT_RPG_BASE_ACTION_H

#include "MovementActions.h"
#include "PlayerRpgInfo.h"

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

protected:
    // Fill botAI->rpgInfo.data with a random grind spot (same map, level band,
    // <=2500yd) via PlayerGrindMgr. Returns true when a spot was found.
    bool SelectRandomGrindPos();
};

#endif
