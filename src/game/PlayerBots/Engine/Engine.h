#ifndef _PLAYERBOT_ENGINE_H
#define _PLAYERBOT_ENGINE_H

#include <vector>
#include <map>
#include <string>
#include "AiObjectContext.h"
#include "Queue.h"
#include "Strategy/Strategy.h"

class PlayerbotAI;
class Unit;

class Engine
{
public:
    Engine(PlayerBotAI* botAI);
    virtual ~Engine() {}

    void Init();
    void Update(uint32 diff);
    void Reset();

    bool DoNextAction();
    void ProcessActions();
    void ProcessTriggers();

    void AddStrategy(Strategy* strategy);
    void RemoveStrategy(Strategy* strategy);
    void RemoveStrategy(uint32 type);

    bool HasStrategy(uint32 type) const;
    bool HasAction(std::string const& name) const;
    bool HasStrategy(std::string const& name) const;

    void SetEnabled(bool enable);
    bool IsEnabled() const { return enabled; }

    ActionQueue& GetQueue() { return queue; }
    AiObjectContext* GetContext() { return context; }

    void ProcessEvent(Event const& event);

    std::vector<Strategy*> const& GetStrategies() const { return strategies; }

private:
    void SortQueue();

    PlayerBotAI* botAI;
    AiObjectContext* context;
    ActionQueue queue;
    std::vector<Strategy*> strategies;
    std::map<uint32, std::vector<Strategy*>> strategiesByType;
    std::map<std::string, Strategy*> strategyMap;
    bool enabled;
    uint32 lastActionTime;
    uint32 lastTriggerTime;
    uint32 actionInterval;
    uint32 triggerInterval;
};

#endif
