#include "ChooseTargetActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "Group.h"
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
    if (!bot || !bot->IsAlive())
        return false;

    // AC pattern: dps assist is for group play only
    // Solo bots use AttackAnythingAction with "grind target"
    if (!bot->GetGroup())
        return false;

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

    // AC pattern: blocked when in combat. "dps assist" handles targeting during combat.
    // "attack anything" is only for peaceful grinding (proactive target acquisition).
    if (bot->IsInCombat())
        return false;

    // AC pattern: grinding only for solo bots or group leaders
    // Group followers follow the leader, don't grind independently
    if (Group* group = bot->GetGroup())
    {
        if (group->GetLeaderGuid() != ObjectGuid(bot->GetGUID()))
            return false;  // not group leader, don't grind
    }

    // Need a valid grind target
    Unit* target = GetTarget();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

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
