/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#ifndef _PLAYERBOT_ACCEPT_RESURRECT_ACTION_H
#define _PLAYERBOT_ACCEPT_RESURRECT_ACTION_H

#include "Action.h"

class PlayerBotAI;

class AcceptResurrectAction : public Action
{
public:
    AcceptResurrectAction(PlayerBotAI* botAI) : Action(botAI, "accept resurrect") {}
    bool Execute(Event event) override;
};

#endif
