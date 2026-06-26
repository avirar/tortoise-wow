/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#ifndef _PLAYERBOT_REVIVE_FROM_CORPSE_ACTIONS_H
#define _PLAYERBOT_REVIVE_FROM_CORPSE_ACTIONS_H

#include "Action.h"

class PlayerBotAI;

class FindCorpseAction : public Action
{
public:
    FindCorpseAction(PlayerBotAI* botAI, std::string const& name = "find corpse")
        : Action(botAI, name) {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class ReviveFromCorpseAction : public Action
{
public:
    ReviveFromCorpseAction(PlayerBotAI* botAI, std::string const& name = "revive from corpse")
        : Action(botAI, name) {}

    bool Execute(Event event) override;
};

class SpiritHealerAction : public Action
{
public:
    SpiritHealerAction(PlayerBotAI* botAI, std::string const& name = "spirit healer")
        : Action(botAI, name) {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

#endif
