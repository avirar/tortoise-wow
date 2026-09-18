/*
 * Rpg actions — R7 L2.
 *
 * Ports of AC mod-playerbots:
 *   - NewRpgStatusUpdateAction (src/Ai/World/Rpg/Action/NewRpgAction.cpp:42-74)
 *     -> RpgStatusUpdateAction: the state-machine driver. Default action (runs
 *     every tick): from IDLE pick a new status; GO_GRIND -> WANDER on arrival;
 *     WANDER/REST -> IDLE when their max duration expires.
 *   - NewRpgGoGrindAction (AC NewRpgAction.cpp:113-116) -> RpgGoGrindAction:
 *     MoveFarTo(rpgInfo.data) — WALK to the grind spot (L1 movement, pathfinding).
 *     AC also checks a quest-giver here (SearchQuestGiverAndAcceptOrReward); that's
 *     the L3+ quest pipeline, deferred.
 *   - NewRpgWanderRandomAction (AC NewRpgAction.cpp:100-105) ->
 *     RpgWanderRandomAction: MoveRandomNear(0, ACTION_DEFAULT) — small local wander
 *     (mill around the current grind area while the base grind strategy kills).
 */
#ifndef _PLAYERBOT_RPG_ACTIONS_H
#define _PLAYERBOT_RPG_ACTIONS_H

#include "RpgBaseAction.h"

class RpgStatusUpdateAction : public RpgBaseAction
{
public:
    explicit RpgStatusUpdateAction(PlayerBotAI* botAI) : RpgBaseAction(botAI) {}
    std::string const getName() { return "rpg status update"; }
    bool Execute(Event event) override;
};

class RpgGoGrindAction : public RpgBaseAction
{
public:
    explicit RpgGoGrindAction(PlayerBotAI* botAI) : RpgBaseAction(botAI) {}
    std::string const getName() { return "rpg go grind"; }
    bool Execute(Event event) override;
};

class RpgWanderRandomAction : public RpgBaseAction
{
public:
    explicit RpgWanderRandomAction(PlayerBotAI* botAI) : RpgBaseAction(botAI) {}
    std::string const getName() { return "rpg wander random"; }
    bool Execute(Event event) override;
};

// L4/L5: AC NewRpgDoQuestAction — the DO_QUEST state machine. Walks to the
// current quest's objective POI (incomplete) or taker POI (complete); the
// kills are done by the base grind strategy (tap model) and the accept/turn-in
// by SearchQuestGiverAndAcceptOrReward (called at the top of Execute).
class RpgDoQuestAction : public RpgBaseAction
{
public:
    explicit RpgDoQuestAction(PlayerBotAI* botAI) : RpgBaseAction(botAI) {}
    std::string const getName() { return "rpg do quest"; }
    bool Execute(Event event) override;

private:
    bool DoIncompleteQuest();
    bool DoCompletedQuest();
};

#endif
