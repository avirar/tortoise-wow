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

class RandomTrigger : public Trigger
{
public:
    RandomTrigger(PlayerBotAI* botAI, std::string const& name, int32 probability = 7);
    bool IsActive() override;

private:
    int32 probability;
    uint32 lastCheck;
};

class NoTargetTrigger : public Trigger
{
public:
    NoTargetTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class NotDpsTargetActiveTrigger : public Trigger
{
public:
    NotDpsTargetActiveTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

class HasTargetTrigger : public Trigger
{
public:
    HasTargetTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

#endif
