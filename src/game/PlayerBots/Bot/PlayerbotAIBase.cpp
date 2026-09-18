#include "PlayerbotAIBase.h"

#include "Logging.h"
#include "Log.h"
#include "World.h"
#include "PerfMonitor.h"
#include "Timer.h"
#include "Unit.h"
#include "Value/Value.h"
#include "AiObjectContext.h"
#include "NonCombatStrategy.h"
#include "CombatStrategy.h"
#include "MeleeCombatStrategy.h"
#include "RangedCombatStrategy.h"
#include "WanderStrategy.h"
#include "GrindingStrategy.h"
#include "LootNonCombatStrategy.h"
#include "DpsAssistStrategy.h"

// R7 L2: RPG state machine
#include "Rpg/RpgStrategy.h"
#include "DeadStrategy.h"
#include "PlayerbotAIConfig.h"

// Class-specific strategies
#include "../Ai/Class/Warrior/Strategy/ArmsWarriorStrategy.h"
#include "../Ai/Class/Warrior/Strategy/FuryWarriorStrategy.h"
#include "../Ai/Class/Warrior/Strategy/TankWarriorStrategy.h"
#include "../Util/SpecDetect.h"
#include "../Ai/Class/Warrior/Strategy/FuryWarriorStrategy.h"
#include "../Ai/Class/Warrior/Strategy/TankWarriorStrategy.h"
#include "../Ai/Class/Warrior/WarriorAiObjectContext.h"
// R2: mage class strategy
#include "../Ai/Class/Mage/Strategy/GenericMageStrategy.h"
#include "../Ai/Class/Warlock/Strategy/GenericWarlockStrategy.h"
#include "../Ai/Class/Priest/Strategy/GenericPriestStrategy.h"
#include "../Ai/Class/Shaman/Strategy/GenericShamanStrategy.h"
#include "../Ai/Class/Paladin/Strategy/GenericPaladinStrategy.h"
#include "../Ai/Class/Druid/Strategy/GenericDruidStrategy.h"

PlayerbotAIBase::PlayerbotAIBase(PlayerBotAI* botAI)
    : botAI(botAI),
      sharedContext(nullptr),
      currentState(BOT_STATE_NON_COMBAT),
      enabled(true),
      nextAICheckDelay(0),
      totalPmo(nullptr),
      grindStrategyActive(false)
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

    // Non-combat engine: NonCombatStrategy + WanderStrategy + DpsAssistStrategy + LootNonCombatStrategy
    // AC AiFactory pattern: GrindingStrategy added ONLY when solo or group leader
    // (AiFactory.cpp:597-612). Added/removed dynamically in UpdateAI() based on group state.
    LOG_DEBUG("playerbots", "[3ENGINE] Creating NON_COMBAT engine");
    engines[BOT_STATE_NON_COMBAT] = new Engine(botAI, sharedContext);
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new NonCombatStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new WanderStrategy(botAI));
    // GrindingStrategy added conditionally in UpdateAI() via string-based API
    grindStrategyActive = false;
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new DpsAssistStrategy(botAI));
    engines[BOT_STATE_NON_COMBAT]->AddStrategy(new LootNonCombatStrategy(botAI));
    // R7 L2: RPG state machine (walk to grind spots, wander, rest). Gated by
    // PlayerBot.RpgEnabled (default 0) until verified; can also be added per-bot
    // at runtime via the agent interface (`adds rpg`). Composes with the existing
    // grind/wander strategies: Rpg provides the "where to be" (walking), the base
    // GrindingStrategy provides the "what to attack".
    if (sPlayerbotAIConfig.rpgEnabled)
    {
        engines[BOT_STATE_NON_COMBAT]->AddStrategy(new RpgStrategy(botAI));
        sLog.outInfo("playerbots: Rpg strategy added to %s (rpgEnabled=1)", botAI->me->GetName());
    }
    engines[BOT_STATE_NON_COMBAT]->Init();
    LOG_DEBUG("playerbots", "[3ENGINE] NON_COMBAT engine done");

    // Combat engine: class-based strategy selection
    LOG_DEBUG("playerbots", "[3ENGINE] Creating COMBAT engine");
    engines[BOT_STATE_COMBAT] = new Engine(botAI, sharedContext);
    uint8 botClass = botAI->me->GetClass();
    if (botClass == CLASS_WARRIOR)
    {
        // P1-3: select the spec strategy from the dominant talent tree
        // (Warrior tabs: 0=Arms, 1=Fury, 2=Protection). Chosen at login;
        // re-talents apply on next login (AC resolves this per tick via
        // GrindTarget values — revisit if bots start re-talenting).
        uint8 specTab = PlayerbotSpec::DetectSpecTab(botAI->me);
        switch (specTab)
        {
            case 1:  engines[BOT_STATE_COMBAT]->AddStrategy(new FuryWarriorStrategy(botAI)); break;
            case 2:  engines[BOT_STATE_COMBAT]->AddStrategy(new TankWarriorStrategy(botAI)); break;
            default: engines[BOT_STATE_COMBAT]->AddStrategy(new ArmsWarriorStrategy(botAI)); break;
        }

        // Register warrior-specific context values
        BuildWarriorAiObjectContext(botAI);

        LOG_DEBUG("playerbots", "[3ENGINE] COMBAT engine: warrior spec tab %u for %s", specTab, botAI->me->GetName());
    }
    else if (botClass == CLASS_PALADIN)
    {
        // R2: paladin strategy (melee + Hammer of Justice CC + self-heal)
        engines[BOT_STATE_COMBAT]->AddStrategy(new GenericPaladinStrategy(botAI));
    }
    else if (botClass == CLASS_DRUID)
    {
        // R2: druid strategy (melee/caster + self-heal; roots via offensive table)
        engines[BOT_STATE_COMBAT]->AddStrategy(new GenericDruidStrategy(botAI));
    }
    else if (botClass == CLASS_MAGE)
    {
        // R2: mage strategy (cast-spell nukes + Polymorph CC; inherits flee-when-close)
        engines[BOT_STATE_COMBAT]->AddStrategy(new GenericMageStrategy(botAI));
    }
    else if (botClass == CLASS_WARLOCK)
    {
        // R2: warlock strategy (cast-spell nukes + Hex CC)
        engines[BOT_STATE_COMBAT]->AddStrategy(new GenericWarlockStrategy(botAI));
    }
    else if (botClass == CLASS_PRIEST)
    {
        // R2: priest strategy (cast-spell nukes + self-heal)
        engines[BOT_STATE_COMBAT]->AddStrategy(new GenericPriestStrategy(botAI));
    }
    else if (botClass == CLASS_SHAMAN)
    {
        // R2: shaman strategy (cast-spell nukes + self-heal)
        engines[BOT_STATE_COMBAT]->AddStrategy(new GenericShamanStrategy(botAI));
    }
    else if (botClass == CLASS_ROGUE)
    {
        // R2: rogue — no low-level class CC; relies on the class offensive
        // table (Eviscerate/Sinister Strike/Garrote). Melee base.
        engines[BOT_STATE_COMBAT]->AddStrategy(new MeleeCombatStrategy(botAI));
    }
    else if (botClass == CLASS_HUNTER)
    {
        // R2: hunter — Auto Shot/Arcane Shot/Serpent Sting via the class
        // offensive table. Ranged base (Auto Shot has a minRange).
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

    // Skip AI processing during server shutdown to avoid accessing
    // destroyed singletons (MovementBroadcaster, etc.)
    if (sWorld.IsStopped())
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

    // R5d normalization: the COMBAT engine deliberately has NO retargeting
    // trigger (CombatStrategy::InitTriggers — retargeting happens on
    // NON_COMBAT via GrindingStrategy), and the only COMBAT→NON_COMBAT exit
    // is DropTargetAction. If a fight ends without a drop (target evaded or
    // died at range, or the idle-relocation sweep cleared the target value),
    // the bot would sit in the COMBAT engine forever pushing only class
    // actions (all USELESS) — never scanning, never attacking, relocating
    // every 120s. The server combat flag is authoritative: if the bot is no
    // longer in combat, the non-combat engine is correct (it rescans and
    // re-attacks via GrindingStrategy).
    if (currentState == BOT_STATE_COMBAT && !bot->IsInCombat())
    {
        ChangeEngine(BOT_STATE_NON_COMBAT);
        // Also clear the dangling "current target": a fight that ended
        // WITHOUT a kill (target evaded/leashed, or died to someone else)
        // leaves a live target set. On NON_COMBAT that blocks the "no
        // target" trigger forever — the bot would stand next to a mob
        // doing nothing. DropTargetAction only clears invalid targets;
        // the normalization is the authoritative transition, so clear here.
        if (sharedContext)
            sharedContext->GetValue<Unit*>("current target")->Set(nullptr);
    }

    // AC AiFactory pattern: GrindingStrategy only when solo or group leader
    // (AiFactory.cpp:597-612). Toggled dynamically as group membership changes.
    {
        bool isGrouped = bot->GetGroup() != nullptr;
        bool isLeader = isGrouped && bot->GetGroup()->IsLeader(bot->GetObjectGuid());
        bool shouldHaveGrind = !isGrouped || isLeader;
        LOG_DEBUG("playerbots", "%s [group] grouped=%d, isLeader=%d, shouldHaveGrind=%d, grindActive=%d",
            bot->GetName(), isGrouped, isLeader, shouldHaveGrind, grindStrategyActive);

        if (shouldHaveGrind && !grindStrategyActive)
        {
            LOG_DEBUG("playerbots", "%s [group] calling AddStrategy(grind, init=true) on NON_COMBAT engine %p",
                bot->GetName(), (void*)engines[BOT_STATE_NON_COMBAT]);
            engines[BOT_STATE_NON_COMBAT]->AddStrategy("grind", true);
            grindStrategyActive = true;
            LOG_DEBUG("playerbots", "%s [group] added grind strategy (solo or leader)", bot->GetName());
        }
        else if (!shouldHaveGrind && grindStrategyActive)
        {
            engines[BOT_STATE_NON_COMBAT]->RemoveStrategy("grind", true);
            grindStrategyActive = false;
            LOG_DEBUG("playerbots", "%s [group] removed grind strategy (grouped, not leader)", bot->GetName());
        }
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

    // Periodic strategy report (every 30s) — debug active strategies on current engine
    {
        static std::map<uint32, uint32> lastReportTime;
        uint32 now = getMSTime();
        uint32 botGuid = bot->GetGUIDLow();
        // P3-4: prune entries for bots that stopped reporting (logout/cleanup)
        // so the static map doesn't grow unboundedly across bot recreation
        if (lastReportTime.size() > 200)
        {
            std::map<uint32, uint32>::iterator it = lastReportTime.begin();
            while (it != lastReportTime.end())
            {
                if (now - it->second > 600000) // 10 min
                    it = lastReportTime.erase(it);
                else
                    ++it;
            }
        }
        if (lastReportTime.find(botGuid) == lastReportTime.end() || now - lastReportTime[botGuid] >= 30000)
        {
            lastReportTime[botGuid] = now;
            Engine* e = engines[currentState];
            std::string stratList;
            if (e)
            {
                for (std::map<std::string, Strategy*>::const_iterator i = e->GetStrategies().begin(); i != e->GetStrategies().end(); ++i)
                    stratList += i->first + ", ";
                if (!stratList.empty())
                    stratList.erase(stratList.size() - 2);
            }
            LOG_DEBUG("playerbots", "%s [strategies] state=%d (%s) engine=%p: [%s]",
                bot->GetName(), currentState,
                currentState == BOT_STATE_COMBAT ? "COMBAT" : currentState == BOT_STATE_NON_COMBAT ? "NON_COMBAT" : "DEAD",
                (void*)e, stratList.c_str());
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
