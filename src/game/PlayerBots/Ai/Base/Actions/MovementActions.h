#ifndef _PLAYERBOT_MOVEMENT_ACTIONS_H
#define _PLAYERBOT_MOVEMENT_ACTIONS_H

#include "Action.h"
#include "Timer.h"

class PlayerBotAI;
class Unit;

class MovementAction : public Action
{
public:
    MovementAction(PlayerBotAI* botAI, std::string const& name = "movement");
    virtual ~MovementAction() {}

    virtual bool Execute(Event event) override;
    virtual std::string const GetHint() { return "move"; }
    virtual std::string const GetEvent() { return "move"; }

    virtual bool isUseful() { return true; }
    virtual bool isPossible() { return true; }

protected:
    bool Follow(Unit* target);
    bool MoveTo(float x, float y, float z);
    bool IsDuplicateMove(float x, float y, float z);
    bool IsMovingAllowed();
    void ClearIdleState();

    uint32 lastMoveTime;
    float lastMoveX;
    float lastMoveY;
    float lastMoveZ;
};

#endif
