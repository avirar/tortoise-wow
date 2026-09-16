/**
 * @file MageActions.h
 * @brief Mage class-specific combat actions (Turtle WoW 1.18.1)
 *
 * Modular pattern matching AC mod-playerbots: each action is a named
 * spell-cast action that resolves the highest known rank and checks
 * cooldowns, mana, and range.
 */
#ifndef _PLAYERBOT_MAGE_ACTIONS_H
#define _PLAYERBOT_MAGE_ACTIONS_H

#include "Action/Action.h"
#include "Timer.h"

class PlayerBotAI;
class Unit;

// ============================================================================
// PolymorphAction — CC the current target (sheep)
// ============================================================================
class CastPolymorphAction : public Action
{
public:
    CastPolymorphAction(PlayerBotAI* botAI);
    virtual ~CastPolymorphAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

#endif
