/**
 * @file MageTriggers.h
 * @brief Mage class-specific combat triggers (Turtle WoW 1.18.1)
 *
 * Modular pattern matching AC mod-playerbots: each trigger checks a
 * specific condition and fires when the condition is met.
 */
#ifndef _PLAYERBOT_MAGE_TRIGGERS_H
#define _PLAYERBOT_MAGE_TRIGGERS_H

#include "Trigger/Trigger.h"

class PlayerBotAI;

// ============================================================================
// CanPolymorphTrigger — target is a valid Polymorph candidate
// ============================================================================
// Vanilla 1.12 rules: cannot polymorph a target higher level than the
// caster (below caster level 30), target must not already be polymorphed.
// Polymorph (118) is a level-9 mage spell — the main low-level CC.
class CanPolymorphTrigger : public Trigger
{
public:
    CanPolymorphTrigger(PlayerBotAI* botAI);
    bool IsActive() override;
};

#endif
