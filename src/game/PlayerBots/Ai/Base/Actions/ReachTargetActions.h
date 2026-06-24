#ifndef _PLAYERBOT_REACH_TARGET_ACTIONS_H
#define _PLAYERBOT_REACH_TARGET_ACTIONS_H

#include "MovementActions.h"

class PlayerBotAI;
class Unit;

class ReachCloseCombatAction : public MovementAction
{
public:
    ReachCloseCombatAction(PlayerBotAI* botAI);
    virtual ~ReachCloseCombatAction() {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful();
};

class ReachSpellCombatAction : public MovementAction
{
public:
    ReachSpellCombatAction(PlayerBotAI* botAI);
    virtual ~ReachSpellCombatAction() {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful();
};

class FleeAction : public MovementAction
{
public:
    FleeAction(PlayerBotAI* botAI);
    virtual ~FleeAction() {}

    virtual bool Execute(Event event) override;
    virtual bool isUseful();
};

#endif
