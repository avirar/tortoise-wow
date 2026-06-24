#ifndef _PLAYERBOT_ATTACK_ACTION_H
#define _PLAYERBOT_ATTACK_ACTION_H

#include "MovementActions.h"

class PlayerBotAI;
class Unit;

class AttackAction : public MovementAction
{
public:
    AttackAction(PlayerBotAI* botAI, std::string const& name = "attack");
    virtual ~AttackAction() {}

    virtual bool Execute(Event event) override;
    virtual std::string const GetTargetName() { return "current target"; }

protected:
    bool DoAttack(Unit* target);
    Unit* GetTarget();
};

class DropTargetAction : public Action
{
public:
    DropTargetAction(PlayerBotAI* botAI);
    virtual ~DropTargetAction() {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;

private:
    uint32 lastDropTime;
};

#endif
