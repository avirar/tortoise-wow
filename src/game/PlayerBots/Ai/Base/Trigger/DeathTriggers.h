/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#ifndef _PLAYERBOT_DEATH_TRIGGERS_H
#define _PLAYERBOT_DEATH_TRIGGERS_H

#include "Trigger.h"

class PlayerBotAI;

class DeadTrigger : public Trigger
{
public:
    DeadTrigger(PlayerBotAI* botAI) : Trigger(botAI, "dead") {}
    bool IsActive() override;
};

class CorpseNearTrigger : public Trigger
{
public:
    CorpseNearTrigger(PlayerBotAI* botAI) : Trigger(botAI, "corpse near", 1000) {}
    bool IsActive() override;
};

class ResurrectRequestTrigger : public Trigger
{
public:
    ResurrectRequestTrigger(PlayerBotAI* botAI) : Trigger(botAI, "resurrect request") {}
    bool IsActive() override;
};

class CanSelfResurrectTrigger : public Trigger
{
public:
    CanSelfResurrectTrigger(PlayerBotAI* botAI) : Trigger(botAI, "can self resurrect") {}
    bool IsActive() override;
};

class FallingFarTrigger : public Trigger
{
public:
    FallingFarTrigger(PlayerBotAI* botAI) : Trigger(botAI, "falling far", 10000) {}
    bool IsActive() override;
};

#endif
