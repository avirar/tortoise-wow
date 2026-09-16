/**
 * @file SelfHealAction.h
 * @brief Combat self-heal action for healer classes (Turtle WoW 1.18.1)
 *
 * Shared across Priest/Shaman/Paladin/Druid: resolves the class's heal
 * spell (highest known rank), checks mana/cooldown, and casts on self.
 * Gated by the "low health" trigger from the class combat strategy so it
 * does not steal priority from the nuke at full health.
 */
#ifndef _PLAYERBOT_SELF_HEAL_ACTION_H
#define _PLAYERBOT_SELF_HEAL_ACTION_H

#include "Action/Action.h"

class PlayerBotAI;

class SelfHealAction : public Action
{
public:
    SelfHealAction(PlayerBotAI* botAI);
    virtual ~SelfHealAction() {}
    virtual bool Execute(Event event) override;
    virtual bool isUseful() override;
};

#endif // _PLAYERBOT_SELF_HEAL_ACTION_H
