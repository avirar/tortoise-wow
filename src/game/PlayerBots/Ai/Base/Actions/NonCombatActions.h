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
    virtual ~SetFacingAction() {}

    virtual bool Execute(Event event) override;
};

class SetBehindAction : public MovementAction
{
public:
    SetBehindAction(PlayerBotAI* botAI);
    virtual ~SetBehindAction() {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful();
};

#endif
