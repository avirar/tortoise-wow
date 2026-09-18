/*
 * Rpg status trigger — R7 L2. See header.
 */

#include "RpgTriggers.h"
#include "PlayerBotAI.h"

bool RpgStatusTrigger::IsActive()
{
    return _status == botAI->rpgInfo.GetStatus();
}
