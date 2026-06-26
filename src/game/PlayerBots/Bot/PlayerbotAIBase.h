#ifndef _PLAYERBOT_AI_BASE_H
#define _PLAYERBOT_AI_BASE_H

#include "PlayerBotAI.h"
#include "Engine/Engine.h"
#include "Engine/AiObjectContext.h"
#include "Engine/Strategy/CustomStrategy.h"
#include "Util/PerfMonitor.h"

enum BotState
{
    BOT_STATE_COMBAT = 0,
    BOT_STATE_NON_COMBAT = 1,
    BOT_STATE_DEAD = 2,
    BOT_STATE_MAX
};

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
    Engine* GetEngine(BotState state) { return state < BOT_STATE_MAX ? engines[state] : nullptr; }
    Engine* GetCurrentEngine() { return currentState < BOT_STATE_MAX ? engines[currentState] : nullptr; }
    BotState GetState() const { return currentState; }

    void SetEnabled(bool enable);
    bool IsEnabled() const { return enabled; }

    // Engine state switching (called from actions, matches AC pattern)
    void ChangeEngine(BotState state);

    // Thread yielding (AC PlayerbotAIBase pattern)
    bool CanUpdateAI() { return nextAICheckDelay == 0; }
    void SetNextCheckDelay(uint32 delay) { nextAICheckDelay = delay; }
    void IncreaseNextCheckDelay(uint32 delay) { nextAICheckDelay += delay; }
    void YieldThread(uint32 delay);
    bool IsActive();

    PlayerBotAI* botAI;
    AiObjectContext* sharedContext;
    Engine* engines[BOT_STATE_MAX];
    BotState currentState;
    bool enabled;
    uint32 nextAICheckDelay;
    PerfMonitorOperation* totalPmo;  // AC pattern: track full tick cycle time
};

#endif
