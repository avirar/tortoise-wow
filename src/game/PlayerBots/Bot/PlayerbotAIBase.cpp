#include "PlayerbotAIBase.h"

#include "Logging.h"
#include "Log.h"
#include "NonCombatStrategy.h"
#include "CombatStrategy.h"
#include "MeleeCombatStrategy.h"
#include "RangedCombatStrategy.h"
#include "WanderStrategy.h"
#include "GrindingStrategy.h"
#include "PlayerbotAIConfig.h"

PlayerbotAIBase::PlayerbotAIBase(PlayerBotAI* botAI)
    : botAI(botAI),
      sharedContext(nullptr),
      currentState(BOT_STATE_NON_COMBAT),
      enabled(true),
      nextAICheckDelay(0)
{
    sLog.outString("[3ENGINE] PlayerbotAIBase constructor: botAI=%p", (void*)botAI);
    for (uint8 i = 0; i < BOT_STATE_MAX; ++i)
        engines[i] = nullptr;
    sLog.outString("[3ENGINE] PlayerbotAIBase constructor: done");
}

PlayerbotAIBase::~PlayerbotAIBase()
{
    sLog.outString("[3ENGINE] PlayerbotAIBase destructor");
    for (uint8 i = 0; i < BOT_STATE_MAX; ++i)
        delete engines[i];
    delete sharedContext;
    sLog.outString("[3ENGINE] PlayerbotAIBase destructor: done");
}

void PlayerbotAIBase::Initialize()
{
    sLog.outString("[3ENGINE] Initialize() START");

    // Create shared context (ONE for all engines, matches AC pattern)
    // AC: aiObjectContext = AiFactory::createAiObjectContext(bot, this);
    sLog.outString("[3ENGINE] Creating shared AiObjectContext");
    sharedContext = new AiObjectContext();
    sharedContext->Init(botAI);
    sLog.outString("[3ENGINE] Shared AiObjectContext created and initialized");

    // Non-combat engine: NonCombatStrategy + WanderStrategy + GrindingStrategy
    sLog.outString("[3ENGINE] Creating NON_COMBAT engine");
    engines[BOT_STATE_NON_COMBAT] = new Engine(botAI, sharedContext);
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new NonCombatStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new WanderStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new GrindingStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->Init();
    sLog.outString("[3ENGINE] NON_COMBAT engine done");

    // Combat engine: class-based strategy selection
    sLog.outString("[3ENGINE] Creating COMBAT engine");
    engines[BOT_STATE_COMBAT] = new Engine(botAI, sharedContext);
    uint8 botClass = botAI->me->GetClass();
    if (botClass == CLASS_WARRIOR || botClass == CLASS_ROGUE ||
        botClass == CLASS_PALADIN || botClass == CLASS_DRUID)
    {
        engines[BOT_STATE_COMBAT]->AddStrategy(new MeleeCombatStrategy(botAI));
    }
    else if (botClass == CLASS_MAGE || botClass == CLASS_PRIEST ||
              botClass == CLASS_WARLOCK || botClass == CLASS_HUNTER ||
              botClass == CLASS_SHAMAN)
    {
        engines[BOT_STATE_COMBAT]->AddStrategy(new RangedCombatStrategy(botAI));
    }
    else
    {
        engines[BOT_STATE_COMBAT]->AddStrategy(new CombatStrategy(botAI));
    }
    engines[BOT_STATE_COMBAT]->Init();
    sLog.outString("[3ENGINE] COMBAT engine done");

    // Dead engine (minimal for now)
    sLog.outString("[3ENGINE] Creating DEAD engine");
    engines[BOT_STATE_DEAD] = new Engine(botAI, sharedContext);
    engines[BOT_STATE_DEAD]->Init();
    sLog.outString("[3ENGINE] DEAD engine done");

    currentState = BOT_STATE_NON_COMBAT;
    sLog.outString("[3ENGINE] Initialize() DONE");
}

void PlayerbotAIBase::UpdateAI(uint32 diff)
{
    if (!enabled || !botAI)
        return;

    // AC pattern: decrement delay and skip if not ready
    if (nextAICheckDelay > diff)
        nextAICheckDelay -= diff;
    else
        nextAICheckDelay = 0;

    if (!CanUpdateAI())
        return;

    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld())
        return;

    // Check if bot died
    if (!bot->IsAlive())
    {
        ChangeEngine(BOT_STATE_DEAD);
        YieldThread(sPlayerbotAIConfig.reactDelay);
        return;
    }

    // Check if bot entered/left combat
    if (bot->IsInCombat() && currentState != BOT_STATE_COMBAT)
    {
        ChangeEngine(BOT_STATE_COMBAT);
    }
    else if (!bot->IsInCombat() && currentState != BOT_STATE_NON_COMBAT)
    {
        ChangeEngine(BOT_STATE_NON_COMBAT);
    }

    Engine* currentEngine = GetCurrentEngine();
    if (!currentEngine)
        return;

    LOG_DEBUG("playerbots", "%s --- AI Tick --- pos=(%.1f,%.1f) state=%u", 
        bot->GetName(), bot->GetPositionX(), bot->GetPositionY(), currentState);
    currentEngine->Update(diff);

    // AC pattern: yield after processing to stagger bot ticks
    YieldThread(sPlayerbotAIConfig.reactDelay);
}

void PlayerbotAIBase::Reset()
{
    for (uint8 i = 0; i < BOT_STATE_MAX; ++i)
    {
        if (engines[i])
            engines[i]->Reset();
    }
}

void PlayerbotAIBase::ChangeEngine(BotState state)
{
    if (currentState == state)
        return;

    Engine* oldEngine = GetCurrentEngine();
    if (oldEngine)
        oldEngine->SetEnabled(false);

    currentState = state;

    Engine* newEngine = GetCurrentEngine();
    if (newEngine)
    {
        newEngine->SetEnabled(true);
        LOG_DEBUG("playerbots", "[PlayerbotAIBase::ChangeEngine] %s changed to state %u",
            botAI->me->GetName(), state);
    }
}

void PlayerbotAIBase::SetEnabled(bool enable)
{
    enabled = enable;
    for (uint8 i = 0; i < BOT_STATE_MAX; ++i)
    {
        if (engines[i])
            engines[i]->SetEnabled(enable);
    }
}

// AC pattern: yield thread with per-bot offset to stagger updates
void PlayerbotAIBase::YieldThread(uint32 delay)
{
    if (nextAICheckDelay < delay)
    {
        // Adding a deterministic per-bot slight offset (0–200 ms) to stagger updates and prevent cpu spikes
        uint32 offset = botAI && botAI->me ? (botAI->me->GetGUIDLow() % 201) : 0;
        nextAICheckDelay = delay + offset;
    }
}

bool PlayerbotAIBase::IsActive()
{
    return nextAICheckDelay < sPlayerbotAIConfig.maxWaitForMove;
}
