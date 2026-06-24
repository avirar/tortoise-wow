#ifndef PLAYERBOT_COMBAT_TRIGGER_H
#define PLAYERBOT_COMBAT_TRIGGER_H

#include "Trigger/Trigger.h"

class PlayerBotAI;

class EnemyOutOfMeleeTrigger : public Trigger
{
public:
    EnemyOutOfMeleeTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class EnemyOutOfSpellTrigger : public Trigger
{
public:
    EnemyOutOfSpellTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class EnemyTooCloseForSpellTrigger : public Trigger
{
public:
    EnemyTooCloseForSpellTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class InvalidTargetTrigger : public Trigger
{
public:
    InvalidTargetTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class NotFacingTargetTrigger : public Trigger
{
public:
    NotFacingTargetTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class NotBehindTargetTrigger : public Trigger
{
public:
    NotBehindTargetTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

#endif
