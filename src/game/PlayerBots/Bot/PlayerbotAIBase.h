#ifndef _PLAYERBOT_AI_BASE_H
#define _PLAYERBOT_AI_BASE_H

#include "PlayerBotAI.h"
#include "Engine/Engine.h"
#include "Engine/Strategy/CustomStrategy.h"

class PlayerbotAIBase
{
public:
    PlayerbotAIBase(PlayerBotAI* botAI);
    virtual ~PlayerbotAIBase();

    virtual void UpdateAI(uint32 diff);
    virtual void Initialize();
    virtual void Reset();

    PlayerBotAI* GetBotAI() const { return botAI; }
    Player* GetBot() const { return botAI ? botAI->me : nullptr; }
    Engine* GetEngine() { return engine; }

    void SetEnabled(bool enable);
    bool IsEnabled() const { return enabled; }

protected:
    PlayerBotAI* botAI;
    Engine* engine;
    bool enabled;
    uint32 lastUpdate;
};

#endif
