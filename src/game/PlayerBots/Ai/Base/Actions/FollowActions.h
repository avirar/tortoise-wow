#ifndef _PLAYERBOT_FOLLOW_ACTIONS_H
#define _PLAYERBOT_FOLLOW_ACTIONS_H

#include "Action/Action.h"

class FollowAction : public Action
{
public:
    FollowAction(PlayerBotAI* botAI) : Action(botAI, "follow") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class FleeToGroupLeaderAction : public Action
{
public:
    FleeToGroupLeaderAction(PlayerBotAI* botAI) : Action(botAI, "flee to group leader") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

#endif
