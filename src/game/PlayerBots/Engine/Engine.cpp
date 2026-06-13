#include "Engine.h"

#include "PerfMonitor.h"
#include "Timer.h"
#include "Logging.h"
#include "PlayerbotAIConfig.h"

#include "Trigger/Trigger.h"
#include "Action/Action.h"

Engine::Engine(PlayerBotAI* botAI)
    : botAI(botAI),
      context(nullptr),
      enabled(true),
      lastActionTime(0),
      lastTriggerTime(0),
      actionInterval(100),
      triggerInterval(100)
{
}

void Engine::Init()
{
    context = new AiObjectContext();
    context->Init(botAI);
}

void Engine::Update(uint32 diff)
{
    if (!enabled)
        return;

    uint32 now = getMSTime();

    if (!lastTriggerTime || now - lastTriggerTime >= triggerInterval)
    {
        lastTriggerTime = now;
        ProcessTriggers();
    }

    if (!lastActionTime || now - lastActionTime >= actionInterval)
    {
        lastActionTime = now;
        ProcessActions();
    }

    if (DoNextAction())
        lastActionTime = getMSTime();
}

void Engine::Reset()
{
    queue.Clear();
    lastActionTime = 0;
    lastTriggerTime = 0;

    if (context)
        context->Reset();

    for (std::vector<Strategy*>::iterator i = strategies.begin(); i != strategies.end(); ++i)
    {
        (*i)->Reset();
    }
}

bool Engine::DoNextAction()
{
    if (queue.Empty())
        return false;

    std::vector<NextAction> const& q = queue.GetQueue();
    if (q.empty())
        return false;

    NextAction next = q[0];
    queue.Clear();

    Strategy* strategy = strategyMap[next.actionName];
    if (!strategy)
        return false;

    ActionNode* actionNode = strategy->GetAction(next.actionName);
    if (!actionNode)
        return false;

    return true;
}

void Engine::ProcessActions()
{
    // Process actions from strategies
    for (std::vector<Strategy*>::iterator i = strategies.begin(); i != strategies.end(); ++i)
    {
        Strategy* strategy = *i;
        std::vector<NextAction> actions = strategy->getDefaultActions();

        for (std::vector<NextAction>::iterator j = actions.begin(); j != actions.end(); ++j)
        {
            queue.Add(*j);
        }
    }

    SortQueue();
}

void Engine::ProcessTriggers()
{
    // Process triggers from strategies
    // Triggers will add actions to the queue
}

void Engine::AddStrategy(Strategy* strategy)
{
    if (!strategy)
        return;

    strategies.push_back(strategy);
    strategyMap[strategy->getName()] = strategy;
    strategiesByType[strategy->GetType()].push_back(strategy);
}

void Engine::RemoveStrategy(Strategy* strategy)
{
    if (!strategy)
        return;

    for (std::vector<Strategy*>::iterator i = strategies.begin(); i != strategies.end(); ++i)
    {
        if (*i == strategy)
        {
            strategies.erase(i);
            break;
        }
    }

    strategyMap.erase(strategy->getName());

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
            strategyMap.erase((*j)->getName());
        }

        for (std::vector<Strategy*>::iterator j = i->second.begin(); j != i->second.end(); ++j)
        {
            for (std::vector<Strategy*>::iterator k = strategies.begin(); k != strategies.end(); ++k)
            {
                if (*k == *j)
                {
                    strategies.erase(k);
                    break;
                }
            }
        }

        i->second.clear();
    }
}

bool Engine::HasStrategy(uint32 type) const
{
    std::map<uint32, std::vector<Strategy*> >::const_iterator i = strategiesByType.find(type);
    return i != strategiesByType.end() && !i->second.empty();
}

bool Engine::HasAction([[maybe_unused]] std::string const& name) const
{
    return strategyMap.find(name) != strategyMap.end();
}

bool Engine::HasStrategy(std::string const& name) const
{
    return strategyMap.find(name) != strategyMap.end();
}

void Engine::SetEnabled(bool enable)
{
    enabled = enable;
}

void Engine::ProcessEvent([[maybe_unused]] Event const& event)
{
    // Process external events
}

void Engine::SortQueue()
{
    // Sort queue by priority (highest first)
    std::vector<NextAction> const& q = queue.GetQueue();
    if (q.size() <= 1)
        return;

    // Simple insertion sort by priority
    std::vector<NextAction> sorted;
    sorted.reserve(q.size());

    for (std::vector<NextAction>::const_iterator i = q.begin(); i != q.end(); ++i)
    {
        bool inserted = false;
        for (size_t j = 0; j < sorted.size(); ++j)
        {
            if (i->priority > sorted[j].priority)
            {
                sorted.insert(sorted.begin() + j, *i);
                inserted = true;
                break;
            }
        }
        if (!inserted)
            sorted.push_back(*i);
    }

    queue.Clear();
    for (std::vector<NextAction>::iterator i = sorted.begin(); i != sorted.end(); ++i)
    {
        queue.Add(*i);
    }
}
