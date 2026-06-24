#ifndef _PLAYERBOT_ENGINE_H
#define _PLAYERBOT_ENGINE_H

#include <vector>
#include <map>
#include <string>
#include "AiObjectContext.h"
#include "Queue.h"
#include "Strategy/Strategy.h"
#include "NamedObjectContext.h"
#include "PlayerBotAI.h"
class Unit;
class TriggerNode;
class Multiplier;

class Engine
{
public:
    Engine(PlayerBotAI* botAI);
    virtual ~Engine() { Reset(); delete context; }

    void Init();
    void Update(uint32 diff);
    void Reset();

    bool DoNextAction();
    void ProcessTriggers();
    void PushDefaultActions();

    void AddStrategy(Strategy* strategy);
    void RemoveStrategy(Strategy* strategy);
    void RemoveStrategy(uint32 type);

    bool HasStrategy(uint32 type) const;
    bool HasStrategy(std::string const& name) const;

    void SetEnabled(bool enable);
    bool IsEnabled() const { return enabled; }

    ActionQueue& GetQueue() { return queue; }
    AiObjectContext* GetContext() { return context; }

    std::map<std::string, Strategy*> const& GetStrategies() const { return strategies; }

protected:
    ActionNode* CreateActionNode(std::string const& name);
    Action* InitializeAction(ActionNode* actionNode);
    bool MultiplyAndPush(std::vector<NextAction> actions, float forceRelevance, bool skipPrerequisites, Event event);
    void PushAgain(ActionNode* actionNode, float relevance, Event event);
    void LogAction(char const* format, ...);

    std::vector<TriggerNode*> triggers;
    std::vector<Multiplier*> multipliers;
    NamedObjectFactoryList<ActionNode> actionNodeFactories;

private:
    PlayerBotAI* botAI;
    AiObjectContext* context;
    ActionQueue queue;
    std::map<std::string, Strategy*> strategies;
    std::map<uint32, std::vector<Strategy*>> strategiesByType;
    bool enabled;
    uint32 lastActionTime;
    uint32 lastTriggerTime;
    uint32 actionInterval;
    uint32 triggerInterval;
    float lastRelevance;
    std::string lastAction;
};

#endif
