#include "GrindTargetValue.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Creature.h"
#include "ServerFacade.h"
#include "PlayerbotAIConfig.h"

GrindTargetValue::GrindTargetValue(PlayerBotAI* botAI)
    : UnitCalculatedValue(botAI, "grind target", 1)
{
}

Unit* GrindTargetValue::Calculate()
{
    // Solo grinding: find nearest valid grind target
    Unit* target = sServerFacade.SelectNearestSafeTarget(bot, sPlayerbotAIConfig.sightDistance);

    if (!target || !target->IsAlive() || bot->IsFriendlyTo(target))
        return nullptr;

    // Skip training dummies
    std::string name = target->GetName();
    if (name.find("Dummy") != std::string::npos ||
        name.find("Charge Target") != std::string::npos ||
        name.find("Melee Target") != std::string::npos ||
        name.find("Ranged Target") != std::string::npos)
        return nullptr;

    return target;
}
