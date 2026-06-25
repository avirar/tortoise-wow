#include "ChooseTargetActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"
#include "AiObjectContext.h"
#include "Value.h"
#include "Logging.h"
#include "Log.h"
#include "Mgr/Item/LootObjectStack.h"

// DpsAssistAction: reads "dps target", attacks it (group assist)
bool DpsAssistAction::isUseful()
{
    // Only useful in battlegrounds when not capturing flag
    // For solo bots, this is not the primary attack action
    return true;
}

// AttackAnythingAction: reads "grind target", attacks it (solo grind)
bool AttackAnythingAction::Execute(Event event)
{
    bool result = AttackAction::Execute(event);
    if (result)
    {
        Unit* grindTarget = GetTarget();
        if (grindTarget)
        {
            LOG_DEBUG("playerbots", "%s [AttackAnything] attacking %s (entry=%u) dist=%.1f",
                bot->GetName(), grindTarget->GetName(),
                grindTarget->GetEntry(),
                sServerFacade.GetDistance2d(bot, grindTarget));
        }
    }
    return result;
}

bool AttackAnythingAction::isUseful()
{
    if (!bot || !bot->IsAlive())
        return false;

    // Don't attack if already in combat — let COMBAT engine handle it
    if (bot->IsInCombat())
        return false;

    // Need a valid grind target
    Unit* target = GetTarget();
    if (!target || !target->IsAlive() || !target->IsInWorld())
    {
        // Debug: scan nearby creatures every 30s when no target found
        static std::map<std::string, uint32> lastDebugTime;
        uint32 now = getMSTime();
        if (lastDebugTime.find(bot->GetName()) == lastDebugTime.end() ||
            now - lastDebugTime[bot->GetName()] > 30000)
        {
            lastDebugTime[bot->GetName()] = now;
            sServerFacade.DebugNearbyCreatures(bot, sPlayerbotAIConfig.sightDistance,
                "AttackAnything::isUseful");
        }
        return false;
    }

    // Skip training dummies
    std::string name = target->GetName();
    if (name.find("Dummy") != std::string::npos ||
        name.find("Charge Target") != std::string::npos ||
        name.find("Melee Target") != std::string::npos ||
        name.find("Ranged Target") != std::string::npos)
        return false;

    return true;
}

bool AttackAnythingAction::isPossible()
{
    return GetTarget() && AttackAction::isPossible();
}

// AggressiveTargetAction: reads "aggressive target", attacks hostile creatures
bool AggressiveTargetAction::isUseful()
{
    return !bot->IsInCombat();
}
