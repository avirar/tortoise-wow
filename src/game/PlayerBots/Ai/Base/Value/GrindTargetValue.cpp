#include "GrindTargetValue.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Creature.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"
#include "Logging.h"

GrindTargetValue::GrindTargetValue(PlayerBotAI* botAI)
    : UnitCalculatedValue(botAI, "grind target", 1)
{
}

Unit* GrindTargetValue::Calculate()
{
    // Solo grinding: find nearest valid grind target
    Unit* target = sServerFacade.SelectNearestSafeTarget(bot, sPlayerbotAIConfig.sightDistance);

    if (!target || !target->IsAlive() || bot->IsFriendlyTo(target))
    {
        LOG_DEBUG("playerbots", "%s [GrindTargetValue] target=%p alive=%u friendly=%u",
            bot->GetName(), (void*)target, target ? target->IsAlive() : 0, target ? bot->IsFriendlyTo(target) : 0);
        return nullptr;
    }

    // Skip training dummies
    std::string name = target->GetName();
    if (name.find("Dummy") != std::string::npos ||
        name.find("Charge Target") != std::string::npos ||
        name.find("Melee Target") != std::string::npos ||
        name.find("Ranged Target") != std::string::npos)
    {
        LOG_DEBUG("playerbots", "%s [GrindTargetValue] skip dummy: %s", bot->GetName(), name.c_str());
        return nullptr;
    }

    LOG_DEBUG("playerbots", "%s [GrindTargetValue] OK: %s (entry=%u) dist=%.1f",
        bot->GetName(), target->GetName(), target->GetEntry(),
        sServerFacade.GetDistance2d(bot, target));
    return target;
}
