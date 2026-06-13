#include "PlayerbotAIBase.h"

#include "Logging.h"
#include "Timer.h"

PlayerbotAIBase::PlayerbotAIBase(PlayerBotAI* botAI)
    : botAI(botAI),
      engine(nullptr),
      enabled(true),
      lastUpdate(0)
{
    engine = new Engine(botAI);
}

PlayerbotAIBase::~PlayerbotAIBase()
{
    delete engine;
}

void PlayerbotAIBase::Initialize()
{
    engine->Init();

    CustomStrategy* customStrategy = new CustomStrategy(botAI);
    engine->AddStrategy(customStrategy);
}

void PlayerbotAIBase::UpdateAI(uint32 diff)
{
    if (!enabled || !botAI)
        return;

    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld())
        return;

    engine->Update(diff);
}

void PlayerbotAIBase::Reset()
{
    if (engine)
        engine->Reset();
}

void PlayerbotAIBase::SetEnabled(bool enable)
{
    enabled = enable;
}
