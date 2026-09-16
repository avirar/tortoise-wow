/**
 * @file PaladinTriggers.h
 * @brief Paladin class-specific combat triggers (Turtle WoW 1.18.1)
 */
#ifndef _PLAYERBOT_PALADIN_TRIGGERS_H
#define _PLAYERBOT_PALADIN_TRIGGERS_H

#include "Trigger/Trigger.h"

class PlayerBotAI;

// ============================================================================
// CanHammerOfJusticeTrigger — target is a valid Hammer of Justice candidate
// ============================================================================
// Hammer of Justice (853) is a level-11 paladin melee stun. Fires when the
// bot has it off cooldown, the target is in melee range, and not already
// stunned.
class CanHammerOfJusticeTrigger : public Trigger
{
public:
    CanHammerOfJusticeTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

#endif // _PLAYERBOT_PALADIN_TRIGGERS_H
