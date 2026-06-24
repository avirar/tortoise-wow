#ifndef _PLAYERBOT_CHOOSE_TARGET_ACTIONS_H
#define _PLAYERBOT_CHOOSE_TARGET_ACTIONS_H

#include "AttackAction.h"

class PlayerBotAI;
class Unit;

class DpsAssistAction : public AttackAction
{
public:
    DpsAssistAction(PlayerBotAI* botAI);
    virtual ~DpsAssistAction() {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful();
    virtual std::string const GetTargetName() { return "dps target"; }
};

class AggressiveTargetAction : public AttackAction
{
public:
    AggressiveTargetAction(PlayerBotAI* botAI);
    virtual ~AggressiveTargetAction() {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful();
    virtual std::string const GetTargetName() { return "aggressive target"; }
};

class AttackAnythingAction : public AttackAction
{
public:
    AttackAnythingAction(PlayerBotAI* botAI);
    virtual ~AttackAnythingAction() {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful();
    virtual bool isPossible();
    virtual std::string const GetTargetName() { return "attack anything"; }
};

#endif
