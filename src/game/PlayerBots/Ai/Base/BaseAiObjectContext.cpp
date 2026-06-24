#include "BaseAiObjectContext.h"

#include "PlayerBotAI.h"
#include "Engine.h"
#include "AiObjectContext.h"

#include "Value/DpsTargetValue.h"
#include "Value/CurrentTargetValue.h"
#include "Value/MasterTargetValue.h"
#include "Value/GroupLeaderValue.h"
#include "Value/InCombatValue.h"

void BuildSharedBaseAiObjectContext(PlayerBotAI* botAI)
{
    Engine* engine = botAI->GetEngine();
    if (!engine)
        return;

    AiObjectContext* context = engine->GetContext();
    if (!context)
        return;

    context->AddValue(new CurrentTargetValue(botAI), "current target");
    context->AddValue(new DpsTargetValue(botAI), "dps target");
    context->AddValue(new MasterTargetValue(botAI), "master target");
    context->AddValue(new GroupLeaderValue(botAI), "group leader");
    context->AddValue(new InCombatValue(botAI), "in combat");
}
