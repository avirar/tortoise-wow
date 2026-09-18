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

// AC mod-playerbots Engine.h:24 — result of a by-name action execution
// (R6.1 agent interface: PlayerbotAI::DoSpecificAction / BotCommandAPI).
enum ActionResult
{
    ACTION_RESULT_UNKNOWN,
    ACTION_RESULT_OK,
    ACTION_RESULT_IMPOSSIBLE,
    ACTION_RESULT_USELESS,
    ACTION_RESULT_FAILED
};

class Engine
{
public:
    Engine(PlayerBotAI* botAI, AiObjectContext* ctx);
    virtual ~Engine() { Reset(); }

    void Init();
    void Update(uint32 diff);
    void Reset();

    bool DoNextAction();
    // AC Engine::ExecuteAction (Bot/Engine/Engine.cpp): run one registered
    // action by name right now (isUseful/isPossible gated), outside the
    // normal trigger/queue cycle. Used by the agent interface (R6.1) and
    // the AC-style PlayerbotAI::DoSpecificAction.
    ActionResult ExecuteAction(std::string const& name, Event event = Event(), std::string const& qualifier = "");
    void ProcessTriggers();
    void PushDefaultActions();

    // Pointer-based (legacy)
    void AddStrategy(Strategy* strategy);
    void RemoveStrategy(Strategy* strategy);
    void RemoveStrategy(uint32 type);

    // AC pattern: string-based, uses AiObjectContext factory
    void AddStrategy(std::string const& name, bool init = true);
    bool RemoveStrategy(std::string const& name, bool init = true);

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
