#ifndef _PLAYERBOT_COMBAT_ACTIONS_H
#define _PLAYERBOT_COMBAT_ACTIONS_H

#include "Action.h"

class PlayerBotAI;

class EnterCombatAction : public Action
{
public:
    EnterCombatAction(PlayerBotAI* botAI);
    virtual ~EnterCombatAction() {}

    virtual bool Execute(Event event) override;
};

class LeaveCombatAction : public Action
{
public:
    LeaveCombatAction(PlayerBotAI* botAI);
    virtual ~LeaveCombatAction() {}

    virtual bool Execute(Event event) override;
};

#endif
