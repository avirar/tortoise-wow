#include "PlayerbotAIBase.h"

#include "Logging.h"
#include "Log.h"
#include "PerfMonitor.h"
#include "Unit.h"
#include "NonCombatStrategy.h"
#include "CombatStrategy.h"
#include "MeleeCombatStrategy.h"
#include "RangedCombatStrategy.h"
#include "WanderStrategy.h"
#include "GrindingStrategy.h"
#include "LootNonCombatStrategy.h"
#include "DpsAssistStrategy.h"
#include "DeadStrategy.h"
#include "PlayerbotAIConfig.h"

// Class-specific strategies
#include "../Ai/Class/Warrior/Strategy/ArmsWarriorStrategy.h"
#include "../Ai/Class/Warrior/Strategy/FuryWarriorStrategy.h"
#include "../Ai/Class/Warrior/Strategy/TankWarriorStrategy.h"
#include "../Ai/Class/Warrior/WarriorAiObjectContext.h"

PlayerbotAIBase::PlayerbotAIBase(PlayerBotAI* botAI)
    : botAI(botAI),
      sharedContext(nullptr),
      currentState(BOT_STATE_NON_COMBAT),
      enabled(true),
      nextAICheckDelay(0),
      totalPmo(nullptr)
{
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerbotAIBase constructor: botAI=%p", (void*)botAI);
    for (uint8 i = 0; i < BOT_STATE_MAX; ++i)
        engines[i] = nullptr;
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerbotAIBase constructor: done");
}

PlayerbotAIBase::~PlayerbotAIBase()
{
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerbotAIBase destructor");
    for (uint8 i = 0; i < BOT_STATE_MAX; ++i)
        delete engines[i];
    delete sharedContext;
    LOG_DEBUG("playerbots", "[3ENGINE] PlayerbotAIBase destructor: done");
}

void PlayerbotAIBase::Initialize()
{
    LOG_DEBUG("playerbots", "[3ENGINE] Initialize() START");

    // Create shared context (ONE for all engines, matches AC pattern)
    // AC: aiObjectContext = AiFactory::createAiObjectContext(bot, this);
    LOG_DEBUG("playerbots", "[3ENGINE] Creating shared AiObjectContext");
    sharedContext = new AiObjectContext();
    sharedContext->Init(botAI);
    LOG_DEBUG("playerbots", "[3ENGINE] Shared AiObjectContext created and initialized");

    // Non-combat engine: NonCombatStrategy + WanderStrategy + GrindingStrategy + DpsAssistStrategy + LootNonCombatStrategy
    // AC pattern: DpsAssistStrategy on NON_COMBAT, fires "dps assist" (50.0f) which outranks "attack anything" (4.0f)
    LOG_DEBUG("playerbots", "[3ENGINE] Creating NON_COMBAT engine");
    engines[BOT_STATE_NON_COMBAT] = new Engine(botAI, sharedContext);
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new NonCombatStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new WanderStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new GrindingStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new DpsAssistStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new LootNonCombatStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->Init();
    LOG_DEBUG("playerbots", "[3ENGINE] NON_COMBAT engine done");

    // Combat engine: class-based strategy selection
    LOG_DEBUG("playerbots", "[3ENGINE] Creating COMBAT engine");
    engines[BOT_STATE_COMBAT] = new Engine(botAI, sharedContext);
    uint8 botClass = botAI->me->GetClass();
    if (botClass == CLASS_WARRIOR)
    {
        // Class-specific warrior strategies (modular, AC pattern)
        // For now, use Arms strategy as default. Spec detection via talents
        // will be added when talent parsing is implemented.
        engines[BOT_STATE_COMBAT]->AddStrategy(new ArmsWarriorStrategy(botAI));

        // Register warrior-specific context values
        BuildWarriorAiObjectContext(botAI);

        LOG_DEBUG("playerbots", "[3ENGINE] COMBAT engine: ArmsWarriorStrategy for warrior");
    }
    else if (botClass == CLASS_ROGUE ||
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
    // AC pattern: "dps assist" in BOTH engines for reactive aggro handling
    engines[BOT_STATE_COMBAT]->AddStrategy(new DpsAssistStrategy(botAI));
    engines[BOT_STATE_COMBAT]->Init();
    LOG_DEBUG("playerbots", "[3ENGINE] COMBAT engine done");

    // Dead engine (minimal for now)
    LOG_DEBUG("playerbots", "[3ENGINE] Creating DEAD engine");
    engines[BOT_STATE_DEAD] = new Engine(botAI, sharedContext);
    engines[BOT_STATE_DEAD]->AddStrategy(new DeadStrategy(botAI));
    engines[BOT_STATE_DEAD]->Init();
    LOG_DEBUG("playerbots", "[3ENGINE] DEAD engine done");

    currentState = BOT_STATE_NON_COMBAT;
    LOG_DEBUG("playerbots", "[3ENGINE] Initialize() DONE");
}

void PlayerbotAIBase::UpdateAI(uint32 diff)
{
    if (!enabled || !botAI)
        return;

    // AC pattern: finish previous FullTick, start new one
    if (totalPmo)
        totalPmo->finish();
    totalPmo = sPlayerbotPerfMonitor.start(PERF_MON_TOTAL, "PlayerbotAIBase::FullTick");

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

    // Engine switching (AC pattern: action-driven, not state-driven)
    // AC switches engine ONLY via actions:
    //   AttackAction::Execute() → ChangeEngine(BOT_STATE_COMBAT)
    //   DropTargetAction::Execute() → ChangeEngine(BOT_STATE_NON_COMBAT)
    // Bot stays on NON_COMBAT when attacked; DpsAssistStrategy handles assist.
    if (!bot->IsAlive())
    {
        ChangeEngine(BOT_STATE_DEAD);
    }
    else if (currentState == BOT_STATE_DEAD)
    {
        // Resurrected: switch back to NON_COMBAT
        ChangeEngine(BOT_STATE_NON_COMBAT);
    }

    // AC pattern: stale-target cleanup (PlayerbotAI.cpp:1514)
    // When bot is on NON_COMBAT but server put it in combat (creature attacked),
    // clear "current target" so "not dps target active" can fire.
    if (currentState == BOT_STATE_NON_COMBAT && bot->IsInCombat())
    {
        Unit* currentTarget = botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
        if (currentTarget)
        {
            botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Set(nullptr);
            LOG_DEBUG("playerbots", "%s [stale-target] cleared 'current target' (was '%s')",
                bot->GetName(), currentTarget->GetName());
        }
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
