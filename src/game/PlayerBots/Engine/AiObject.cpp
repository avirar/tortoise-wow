#include "AiObject.h"

#include "PlayerBotAI.h"
#include "Player.h"

AiObject::AiObject(PlayerBotAI* botAI)
    : PlayerbotAIAware(botAI)
{
    bot = botAI ? botAI->me : nullptr;
    context = nullptr;
    chat = nullptr;
}

Player* AiObject::GetMaster()
{
    return nullptr;
}
