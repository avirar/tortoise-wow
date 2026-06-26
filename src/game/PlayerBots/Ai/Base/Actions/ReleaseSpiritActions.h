/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#ifndef _PLAYERBOT_RELEASE_SPIRIT_ACTIONS_H
#define _PLAYERBOT_RELEASE_SPIRIT_ACTIONS_H

#include "Action.h"
#include "ReviveFromCorpseActions.h"

class PlayerBotAI;

class ReleaseSpiritAction : public Action
{
public:
    ReleaseSpiritAction(PlayerBotAI* botAI, std::string const& name = "release")
        : Action(botAI, name) {}

    bool Execute(Event event) override;
};

class AutoReleaseSpiritAction : public ReleaseSpiritAction
{
public:
    AutoReleaseSpiritAction(PlayerBotAI* botAI, std::string const& name = "auto release")
        : ReleaseSpiritAction(botAI, name) {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class RepopAction : public SpiritHealerAction
{
public:
    RepopAction(PlayerBotAI* botAI, std::string const& name = "repop")
        : SpiritHealerAction(botAI, name) {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class SelfResurrectAction : public Action
{
public:
    SelfResurrectAction(PlayerBotAI* botAI) : Action(botAI, "self resurrect") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

#endif
