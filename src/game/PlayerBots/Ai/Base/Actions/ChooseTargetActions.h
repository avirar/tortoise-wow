#ifndef _PLAYERBOT_CHOOSE_TARGET_ACTIONS_H
#define _PLAYERBOT_CHOOSE_TARGET_ACTIONS_H

#include "AttackAction.h"

class PlayerBotAI;
class Unit;

class DpsAssistAction : public AttackAction
{
public:
    DpsAssistAction(PlayerBotAI* botAI) : AttackAction(botAI, "dps assist") {}
    bool isUseful() override;
    std::string const GetTargetName() override { return "dps target"; }
};

class AggressiveTargetAction : public AttackAction
{
public:
    AggressiveTargetAction(PlayerBotAI* botAI) : AttackAction(botAI, "aggressive target") {}
    bool isUseful() override;
    std::string const GetTargetName() override { return "aggressive target"; }
};

class AttackAnythingAction : public AttackAction
{
public:
    AttackAnythingAction(PlayerBotAI* botAI) : AttackAction(botAI, "attack anything") {}
    bool Execute(Event event) override;
    bool isUseful() override;
    bool isPossible() override;
    std::string const GetTargetName() override { return "grind target"; }
};

#endif
