/**
 * @file PaladinActions.h
 * @brief Paladin class-specific combat actions (Turtle WoW 1.18.1)
 */
#ifndef _PLAYERBOT_PALADIN_ACTIONS_H
#define _PLAYERBOT_PALADIN_ACTIONS_H

#include "Action/Action.h"

class PlayerBotAI;

// ============================================================================
// CastHammerOfJusticeAction — melee stun the current target
// ============================================================================
class CastHammerOfJusticeAction : public Action
{
public:
    CastHammerOfJusticeAction(PlayerBotAI* botAI);
    virtual ~CastHammerOfJusticeAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

#endif // _PLAYERBOT_PALADIN_ACTIONS_H
