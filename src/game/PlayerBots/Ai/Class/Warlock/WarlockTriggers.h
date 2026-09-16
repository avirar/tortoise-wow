/**
 * @file WarlockTriggers.h
 * @brief Warlock class-specific combat triggers (Turtle WoW 1.18.1)
 */
#ifndef _PLAYERBOT_WARLOCK_TRIGGERS_H
#define _PLAYERBOT_WARLOCK_TRIGGERS_H

#include "Trigger/Trigger.h"

class PlayerBotAI;

// ============================================================================
// CanFearTrigger — target is a valid Fear candidate
// ============================================================================
// Fear (5782) is the warlock's learnable low-level CC (bots actually have
// it). Fires when the bot has Fear, it is off cooldown, and the target is
// alive and not already feared.
// (Hex 11641 is spellLevel 30 and absent from skill_line_ability — the
// autolearn loop never grants it — so Fear is the usable CC.)
class CanFearTrigger : public Trigger
{
public:
    CanFearTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

#endif // _PLAYERBOT_WARLOCK_TRIGGERS_H
