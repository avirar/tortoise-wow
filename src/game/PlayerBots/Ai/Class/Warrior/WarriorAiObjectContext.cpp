/**
 * @file WarriorAiObjectContext.cpp
 * @brief Warrior class-specific context value registration
 */
#include "WarriorAiObjectContext.h"

#include "PlayerBotAI.h"
#include "AiObjectContext.h"

void BuildWarriorAiObjectContext(PlayerBotAI* botAI)
{
    if (!botAI || !botAI->GetAiObjectContext())
        return;

    AiObjectContext* context = botAI->GetAiObjectContext();

    // Register warrior-specific values
    context->AddValue(new WarriorStanceValue(botAI), "warrior stance");
    context->AddValue(new WarriorRageValue(botAI), "warrior rage");
    context->AddValue(new WarriorHasBattleShoutValue(botAI), "warrior has battle shout");
    context->AddValue(new WarriorHasRendValue(botAI), "warrior has rend");
    context->AddValue(new WarriorHasSunderArmorValue(botAI), "warrior has sunder armor");
    context->AddValue(new WarriorHasThunderClapValue(botAI), "warrior has thunder clap");
    context->AddValue(new WarriorTargetHealthPercentValue(botAI), "warrior target health percent");
}
