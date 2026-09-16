/**
 * @file WarlockActions.h
 * @brief Warlock class-specific combat actions (Turtle WoW 1.18.1)
 */
#ifndef _PLAYERBOT_WARLOCK_ACTIONS_H
#define _PLAYERBOT_WARLOCK_ACTIONS_H

#include "Action/Action.h"

class PlayerBotAI;

// ============================================================================
// CastFearAction — CC the current target (fear)
// ============================================================================
class CastFearAction : public Action
{
public:
    CastFearAction(PlayerBotAI* botAI);
    virtual ~CastFearAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

#endif // _PLAYERBOT_WARLOCK_ACTIONS_H
