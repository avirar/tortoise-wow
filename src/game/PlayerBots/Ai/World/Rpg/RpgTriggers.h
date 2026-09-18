/*
 * Rpg status trigger — R7 L2. Port of AC `NewRpgStatusTrigger`
 * (src/Ai/World/Rpg/Trigger/NewRpgTriggers.h): a single parameterized trigger
 * (node names "go grind status", "wander random status", ...) that is active when
 * the bot's current RPG status matches the one it was constructed with.
 */
#ifndef _PLAYERBOT_RPG_TRIGGERS_H
#define _PLAYERBOT_RPG_TRIGGERS_H

#include "Trigger/Trigger.h"
#include "PlayerRpgInfo.h"

class RpgStatusTrigger : public Trigger
{
public:
    RpgStatusTrigger(PlayerBotAI* botAI, PlayerRpgStatus status = RPG_IDLE)
        : Trigger(botAI, "rpg status"), _status(status)
    {
    }
    bool IsActive() override;

private:
    PlayerRpgStatus _status;
};

#endif
