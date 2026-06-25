#ifndef _PLAYERBOT_NONCOMBAT_ACTIONS_H
#define _PLAYERBOT_NONCOMBAT_ACTIONS_H

#include "Action.h"
#include "MovementActions.h"

class PlayerBotAI;
class Unit;

class SetFacingAction : public Action
{
public:
    SetFacingAction(PlayerBotAI* botAI);
    bool Execute(Event event) override;
};

class SetBehindAction : public MovementAction
{
public:
    SetBehindAction(PlayerBotAI* botAI);
    bool Execute(Event event) override;
    bool isUseful() override;
};

class EatAction : public Action
{
public:
    EatAction(PlayerBotAI* botAI);
    bool Execute(Event event) override;
    bool isUseful() override;
    bool isPossible() override;
};

class DrinkAction : public Action
{
public:
    DrinkAction(PlayerBotAI* botAI);
    bool Execute(Event event) override;
    bool isUseful() override;
    bool isPossible() override;
};

#endif
