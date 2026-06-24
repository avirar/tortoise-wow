#ifndef PLAYERBOT_HEALTH_TRIGGER_H
#define PLAYERBOT_HEALTH_TRIGGER_H

#include "Trigger/Trigger.h"

class PlayerBotAI;

class LowHealthTrigger : public Trigger
{
public:
    LowHealthTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class MediumHealthTrigger : public Trigger
{
public:
    MediumHealthTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class LowManaTrigger : public Trigger
{
public:
    LowManaTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class HighManaTrigger : public Trigger
{
public:
    HighManaTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

#endif
