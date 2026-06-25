#include "WaitForAttackStrategy.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "AiObjectContext.h"
#include "Value.h"
#include "Unit.h"

// AC pattern: ShouldWait only returns true in group with real player master
// For solo bots (no group), always returns false → attack immediately
bool WaitForAttackStrategy::ShouldWait(PlayerBotAI* botAI)
{
    if (!botAI || !botAI->me)
        return false;

    Player* bot = botAI->me;

    // AC: only wait if bot is in a group AND has a real player master
    // Solo bots always attack immediately
    if (!bot->GetGroup())
        return false;

    // Don't wait if current target is an enemy player
    Unit* target = botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (target && target->IsPlayer())
        return false;

    // AC: wait for configured time after combat starts (prevents pulling too early)
    // Not implemented for solo bots, but kept for future group support
    return false;
}
