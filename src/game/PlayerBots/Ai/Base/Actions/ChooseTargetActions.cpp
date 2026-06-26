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
// AC pattern: minimal check, just return true (DpsTargetValue handles target finding)
bool DpsAssistAction::isUseful()
{
    if (!bot || !bot->IsAlive())
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
    {
        LOG_DEBUG("playerbots", "%s [AttackAnything::isUseful] -> false (no bot/dead)", bot ? bot->GetName() : "null");
        return false;
    }

    // AC pattern: blocked when in combat. "dps assist" handles targeting during combat.
    // "attack anything" is only for peaceful grinding (proactive target acquisition).
    if (bot->IsInCombat())
    {
        LOG_DEBUG("playerbots", "%s [AttackAnything::isUseful] -> false (in combat)", bot->GetName());
        return false;
    }

    // Need a valid grind target
    Unit* target = GetTarget();
    if (!target || !target->IsAlive() || !target->IsInWorld())
    {
        LOG_DEBUG("playerbots", "%s [AttackAnything::isUseful] -> false (no valid grind target, target=%s)",
            bot->GetName(), target ? target->GetName() : "null");
        return false;
    }

    // Skip training dummies
    std::string name = target->GetName();
    if (name.find("Dummy") != std::string::npos ||
        name.find("Charge Target") != std::string::npos ||
        name.find("Melee Target") != std::string::npos ||
        name.find("Ranged Target") != std::string::npos)
    {
        LOG_DEBUG("playerbots", "%s [AttackAnything::isUseful] -> false (dummy: %s)",
            bot->GetName(), name.c_str());
        return false;
    }

    LOG_DEBUG("playerbots", "%s [AttackAnything::isUseful] -> true (target: %s, entry=%u)",
        bot->GetName(), target->GetName(), target->GetEntry());
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
