#include "Engine.h"

#include <cstdarg>
#include <unordered_map>

#include "PerfMonitor.h"
#include "Timer.h"
#include "Logging.h"
#include "Log.h"
#include "PlayerbotAIConfig.h"
#include "PlayerBotAI.h"

#include "Trigger/Trigger.h"
#include "Trigger/TriggerNode.h"
#include "Action/Action.h"

Engine::Engine(PlayerBotAI* botAI, AiObjectContext* ctx)
    : botAI(botAI),
      context(ctx),
      enabled(true),
      lastActionTime(0),
      lastTriggerTime(0),
      actionInterval(100),
      triggerInterval(100),
      lastRelevance(0.0f)
{
    LOG_DEBUG("playerbots", "[3ENGINE] Engine constructor: botAI=%p", (void*)botAI);
}

void Engine::Init()
{
    LOG_DEBUG("playerbots", "[3ENGINE] Engine::Init() START, context=%p", (void*)context);
    // Reset engine-local state first (AC pattern) — does NOT touch shared context
    Reset();

    // Context is shared across all 3 engines, initialized in PlayerbotAIBase::Initialize()
    // Guard kept for safety but should always be already initialized
    if (context && !context->IsInitialized())
        context->Init(botAI);

    for (std::map<std::string, Strategy*>::iterator i = strategies.begin(); i != strategies.end(); ++i)
    {
        Strategy* strategy = i->second;
        if (!strategy)
            continue;

        strategy->InitTriggers(triggers);
        strategy->InitMultipliers(multipliers);
        for (auto const& iter : strategy->actionNodeFactories.creators)
        {
            actionNodeFactories.creators[iter.first] = iter.second;
        }
    }
}

void Engine::Update(uint32 /*diff*/)
{
    if (!enabled)
        return;

    DoNextAction();
}

void Engine::Reset()
{
    queue.Clear();
    lastActionTime = 0;
    lastTriggerTime = 0;
    lastRelevance = 0.0f;
    lastAction.clear();

    // DO NOT call context->Reset() — context is shared across all 3 engines (AC pattern)
    // Resetting shared context would wipe "current target" etc. for other engines

    for (std::vector<TriggerNode*>::iterator i = triggers.begin(); i != triggers.end(); ++i)
    {
        delete *i;
    }
    triggers.clear();

    for (std::vector<Multiplier*>::iterator i = multipliers.begin(); i != multipliers.end(); ++i)
    {
        delete *i;
    }
    multipliers.clear();

    for (std::map<std::string, Strategy*>::iterator i = strategies.begin(); i != strategies.end(); ++i)
    {
        Strategy* strategy = i->second;
        if (strategy)
            strategy->Reset();
    }
}

bool Engine::DoNextAction()
{
    LOG_DEBUG("playerbots", "[Engine::DoNextAction] botAI=%p, me=%p", (void*)botAI, (void*)(botAI->me));
    LogAction("--- AI Tick ---");

    bool actionExecuted = false;
    ActionBasket* basket = nullptr;

    ProcessTriggers();
    PushDefaultActions();

    LOG_DEBUG("playerbots", "[DoNextAction] queue size=%u", queue.Size());

    uint32 iterations = 0;
    uint32 iterationsPerTick = queue.Size() * 2;
    if (iterationsPerTick < 1)
        iterationsPerTick = 1;

    while (++iterations <= iterationsPerTick)
    {
        basket = queue.Peek();
        if (!basket)
        {
            LOG_DEBUG("playerbots", "%s [DoNextAction] queue empty after %u iterations", botAI->me->GetName(), iterations);
            break;
        }

        float relevance = basket->getRelevance();
        bool skipPrerequisites = basket->isSkipPrerequisites();

        Event event = basket->getEvent();
        ActionNode* actionNode = queue.Pop();
        if (!actionNode)
            break;

        Action* action = InitializeAction(actionNode);

        if (!action)
        {
            LogAction("A:%s - UNKNOWN", actionNode->getName().c_str());
        }
        else if (action->isUseful())
        {
            for (std::vector<Multiplier*>::iterator mi = multipliers.begin(); mi != multipliers.end(); ++mi)
            {
                relevance *= (*mi)->GetValue(action);
                action->setRelevance(relevance);

                if (relevance <= 0)
                {
                    LogAction("Multiplier %s made action %s useless", (*mi)->getName().c_str(), action->getName().c_str());
                    break;
                }
            }

            if (action->isPossible() && relevance > 0)
            {
                if (!skipPrerequisites)
                {
                    LogAction("A:%s - PREREQ", action->getName().c_str());

                    if (MultiplyAndPush(actionNode->getPrerequisites(), relevance + 0.002f, false, event))
                    {
                        PushAgain(actionNode, relevance + 0.001f, event);
                        continue;
                    }
                }

                PerfMonitorOperation* pmo = sPlayerbotPerfMonitor.start(PERF_MON_ACTION, action->getName(), &context->performanceStack);
                actionExecuted = action->Execute(event);
                if (pmo) pmo->finish();

                if (actionExecuted)
                {
                    LogAction("A:%s - OK", action->getName().c_str());
                    MultiplyAndPush(actionNode->getContinuers(), relevance, false, event);
                    lastRelevance = relevance;
                    delete actionNode;
                    break;
                }
                else
                {
                    LogAction("A:%s - FAILED", action->getName().c_str());
                    MultiplyAndPush(actionNode->getAlternatives(), relevance + 0.003f, false, event);
                }
            }
            else
            {
                LogAction("A:%s - IMPOSSIBLE", action->getName().c_str());
                MultiplyAndPush(actionNode->getAlternatives(), relevance + 0.003f, false, event);
            }
        }
        else
        {
            LogAction("A:%s - USELESS", action->getName().c_str());
            lastRelevance = relevance;
        }

        delete actionNode;
    }

    if (!actionExecuted)
        LogAction("no actions executed");

    queue.RemoveExpired();

    return actionExecuted;
}

void Engine::ProcessTriggers()
{
    std::unordered_map<Trigger*, Event> fires;
    uint32 now = getMSTime();

    for (std::vector<TriggerNode*>::iterator i = triggers.begin(); i != triggers.end(); ++i)
    {
        TriggerNode* node = *i;
        if (!node)
            continue;

        Trigger* trigger = node->getTrigger();
        if (!trigger)
        {
            trigger = context ? context->GetTrigger(node->getName()) : nullptr;
            node->setTrigger(trigger);
        }

        if (!trigger)
            continue;

        if (fires.find(trigger) != fires.end())
            continue;

        if (trigger->needCheck(now))
        {
            PerfMonitorOperation* pmo = sPlayerbotPerfMonitor.start(PERF_MON_TRIGGER, trigger->getName(), &context->performanceStack);
            Event evt = trigger->Check();
            if (pmo) pmo->finish();
            if (!evt.IsEmpty())
            {
                fires[trigger] = evt;
                LogAction("T:%s", trigger->getName().c_str());
            }
        }
    }

    for (std::vector<TriggerNode*>::iterator i = triggers.begin(); i != triggers.end(); ++i)
    {
        TriggerNode* node = *i;
        Trigger* trigger = node->getTrigger();
        if (fires.find(trigger) == fires.end())
            continue;

        Event evt = fires[trigger];
        MultiplyAndPush(node->getHandlers(), 0.0f, false, evt);
    }

    for (std::vector<TriggerNode*>::iterator i = triggers.begin(); i != triggers.end(); ++i)
    {
        if (Trigger* trigger = (*i)->getTrigger())
            trigger->Reset();
    }
}

void Engine::PushDefaultActions()
{
    for (std::map<std::string, Strategy*>::iterator i = strategies.begin(); i != strategies.end(); ++i)
    {
        Strategy* strategy = i->second;
        if (!strategy)
            continue;

        std::vector<NextAction> actions = strategy->getDefaultActions();
        Event emptyEvent;
        MultiplyAndPush(actions, 0.0f, false, emptyEvent);
    }
}

ActionNode* Engine::CreateActionNode(std::string const& name)
{
    ActionNode* node = actionNodeFactories.GetContextObject(name, botAI);
    if (node)
        return node;

    return new ActionNode(name, {}, {}, {});
}

Action* Engine::InitializeAction(ActionNode* actionNode)
{
    Action* action = actionNode->getAction();
    if (!action)
    {
        action = context ? context->GetAction(actionNode->getName()) : nullptr;
        actionNode->setAction(action);
        if (!action)
            LOG_DEBUG("playerbots", "%s [InitializeAction] FAILED to find action '%s', ctx=%p", botAI->me->GetName(), actionNode->getName().c_str(), (void*)context);
    }

    return action;
}

bool Engine::MultiplyAndPush(std::vector<NextAction> actions, float forceRelevance, bool skipPrerequisites, Event event)
{
    bool pushed = false;

    for (std::vector<NextAction>::iterator ai = actions.begin(); ai != actions.end(); ++ai)
    {
        ActionNode* action = CreateActionNode(ai->getName());
        if (!action)
            continue;

        InitializeAction(action);

        float k = ai->getRelevance();
        if (forceRelevance > 0.0f)
            k = forceRelevance;

        if (k >= 0)
        {
            LogAction("PUSH:%s - %f", action->getName().c_str(), k);
            queue.Push(new ActionBasket(action, k, skipPrerequisites, event));
            pushed = true;
        }
        else
        {
            delete action;
        }
    }

    return pushed;
}

void Engine::PushAgain(ActionNode* actionNode, float relevance, Event event)
{
    std::vector<NextAction> nextAction;
    nextAction.push_back(NextAction(actionNode->getName(), relevance));

    MultiplyAndPush(nextAction, relevance, true, event);
    delete actionNode;
}

void Engine::AddStrategy(Strategy* strategy)
{
    if (!strategy)
        return;

    strategies[strategy->getName()] = strategy;
    strategiesByType[strategy->GetType()].push_back(strategy);
}

void Engine::RemoveStrategy(Strategy* strategy)
{
    if (!strategy)
        return;

    strategies.erase(strategy->getName());

    for (std::map<uint32, std::vector<Strategy*> >::iterator i = strategiesByType.begin(); i != strategiesByType.end(); ++i)
    {
        for (std::vector<Strategy*>::iterator j = i->second.begin(); j != i->second.end(); ++j)
        {
            if (*j == strategy)
            {
                i->second.erase(j);
                break;
            }
        }
    }
}

void Engine::RemoveStrategy(uint32 type)
{
    std::map<uint32, std::vector<Strategy*> >::iterator i = strategiesByType.find(type);
    if (i != strategiesByType.end())
    {
        for (std::vector<Strategy*>::iterator j = i->second.begin(); j != i->second.end(); ++j)
        {
            strategies.erase((*j)->getName());
        }
        i->second.clear();
    }
}

// AC pattern: string-based strategy management via AiObjectContext factory
void Engine::AddStrategy(std::string const& name, bool init)
{
    RemoveStrategy(name, init);  // remove existing first (AC pattern)

    Strategy* strategy = context->GetStrategy(name);
    LOG_DEBUG("playerbots", "[Engine] AddStrategy(%s) on engine %p, factory returned %p, strategies.size=%zu",
        name.c_str(), (void*)this, (void*)strategy, strategies.size());

    if (strategy)
    {
        LogAction("S:+%s", strategy->getName().c_str());
        strategies[strategy->getName()] = strategy;
        LOG_DEBUG("playerbots", "[Engine] AddStrategy(%s) OK, now %zu strategies", name.c_str(), strategies.size());
    }
    else
    {
        LOG_DEBUG("playerbots", "[Engine] AddStrategy(%s) FAILED: factory returned null", name.c_str());
    }
    if (init)
        Init();
}

bool Engine::RemoveStrategy(std::string const& name, bool init)
{
    std::map<std::string, Strategy*>::iterator it = strategies.find(name);
    if (it == strategies.end())
    {
        LOG_DEBUG("playerbots", "[Engine] RemoveStrategy(%s) NOT FOUND, strategies=%zu", name.c_str(), strategies.size());
        return false;
    }

    strategies.erase(it);
    LOG_DEBUG("playerbots", "[Engine] RemoveStrategy(%s) OK, now %zu strategies", name.c_str(), strategies.size());
    if (init)
        Init();
    return true;
}

bool Engine::HasStrategy(uint32 type) const
{
    for (std::map<std::string, Strategy*>::const_iterator i = strategies.begin(); i != strategies.end(); ++i)
    {
        if (i->second->GetType() & type)
            return true;
    }
    return false;
}

bool Engine::HasStrategy(std::string const& name) const
{
    return strategies.find(name) != strategies.end();
}

void Engine::SetEnabled(bool enable)
{
    enabled = enable;
}

void Engine::LogAction(char const* format, ...)
{
    if (!botAI || !botAI->me)
        return;

    char buf[1024];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    lastAction += "|";
    lastAction += buf;
    if (lastAction.size() > 512)
    {
        lastAction = lastAction.substr(512);
        size_t pos = lastAction.find("|");
        lastAction = (pos == std::string::npos ? "" : lastAction.substr(pos));
    }

    LOG_DEBUG("playerbots", "%s %s", botAI->me->GetName(), buf);
}
