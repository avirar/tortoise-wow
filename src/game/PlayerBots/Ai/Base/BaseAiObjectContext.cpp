#include "BaseAiObjectContext.h"

#include "PlayerBotAI.h"
#include "Engine.h"
#include "AiObjectContext.h"

#include "Value/DpsTargetValue.h"
#include "Value/CurrentTargetValue.h"
#include "Value/MasterTargetValue.h"
#include "Value/GroupLeaderValue.h"
#include "Value/InCombatValue.h"
#include "Value/ItemUsageValue.h"
#include "Value/LootStrategyValue.h"
#include "Value/AvailableLootValue.h"

void BuildSharedBaseAiObjectContext(PlayerBotAI* botAI, AiObjectContext* context)
{
    if (!context)
        return;

    context->AddValue(new CurrentTargetValue(botAI), "current target");
    context->AddValue(new DpsTargetValue(botAI), "dps target");
    context->AddValue(new MasterTargetValue(botAI), "master target");
    context->AddValue(new GroupLeaderValue(botAI), "group leader");
    context->AddValue(new InCombatValue(botAI), "in combat");
    context->AddValue(new ItemUsageValue(botAI), "item usage");
    context->AddValue(new ItemUpgradeValue(botAI), "item upgrade");
    context->AddValue(new LootStrategyValue(botAI), "loot strategy");
    context->AddValue(new AvailableLootValue(botAI), "available loot");
    context->AddValue(new LootTargetValue(botAI), "loot target");
    context->AddValue(new HasAvailableLootValue(botAI), "has available loot");
    context->AddValue(new CanLootValue(botAI), "can loot");
    context->AddValue(new BagSpaceValue(botAI), "bag space");
}
