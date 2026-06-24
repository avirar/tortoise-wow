#ifndef _PLAYERBOT_ACTION_H
#define _PLAYERBOT_ACTION_H

#include <vector>
#include <string>
#include "AiObject.h"
#include "Event.h"
#include "Timer.h"

class PlayerBotAI;
class Unit;

class NextAction
{
public:
    NextAction(std::string const& name, float relevance = 0.0f)
        : relevance(relevance), name(name) {}
    NextAction(NextAction const& o) : relevance(o.relevance), name(o.name) {}

    std::string const getName() { return name; }
    float getRelevance() { return relevance; }

    static std::vector<NextAction> merge(std::vector<NextAction> const& what, std::vector<NextAction> const& with)
    {
        std::vector<NextAction> result;
        for (NextAction const& action : what)
            result.push_back(action);
        for (NextAction const& action : with)
            result.push_back(action);
        return result;
    }

private:
    float relevance;
    std::string name;
};

class Action : public AiNamedObject
{
public:
    Action(PlayerBotAI* botAI, std::string const& name = "action");
    virtual ~Action() {}

    virtual bool Execute(Event event);

    virtual bool isUseful() { return true; }
    virtual bool isPossible() { return true; }

    virtual std::vector<NextAction> getPrerequisites() { return {}; }
    virtual std::vector<NextAction> getAlternatives() { return {}; }
    virtual std::vector<NextAction> getContinuers() { return {}; }

    void MakeVerbose() { verbose = true; }
    void setRelevance(float relevance1) { relevance = relevance1; }
    virtual float getRelevance() { return relevance; }

    virtual std::string const GetHint() { return "execute"; }
    virtual std::string const GetEvent() { return "execute"; }

    bool needRepeat(uint32 now);

protected:
    bool verbose;
    float relevance;
    int32 repeatInterval;
    uint32 lastRepeatTime;
};

class ActionNode
{
public:
    ActionNode(
        std::string const& name,
        std::vector<NextAction> const& P,
        std::vector<NextAction> const& A,
        std::vector<NextAction> const& C
    ) : name(name), action(nullptr), continuers(C), alternatives(A), prerequisites(P) {}

    virtual ~ActionNode() {}

    Action* getAction() { return action; }
    void setAction(Action* act) { this->action = act; }
    std::string const getName() { return name; }

    std::vector<NextAction> getContinuers()
    {
        if (!action)
            return continuers;
        return NextAction::merge(continuers, action->getContinuers());
    }

    std::vector<NextAction> getAlternatives()
    {
        if (!action)
            return alternatives;
        return NextAction::merge(alternatives, action->getAlternatives());
    }

    std::vector<NextAction> getPrerequisites()
    {
        if (!action)
            return prerequisites;
        return NextAction::merge(prerequisites, action->getPrerequisites());
    }

private:
    std::string name;
    Action* action;
    std::vector<NextAction> continuers;
    std::vector<NextAction> alternatives;
    std::vector<NextAction> prerequisites;
};

class ActionBasket
{
public:
    ActionBasket(ActionNode* act, float rel, bool skipPrereq, Event evt);

    virtual ~ActionBasket() {}

    float getRelevance() { return relevance; }
    ActionNode* getAction() { return action; }
    Event getEvent() { return event; }
    bool isSkipPrerequisites() { return skipPrerequisites; }
    void AmendRelevance(float k) { relevance *= k; }
    void setRelevance(float rel) { relevance = rel; }
    bool isExpired(uint32_t msecs);

private:
    ActionNode* action;
    float relevance;
    bool skipPrerequisites;
    Event event;
    uint32_t created;
};

#endif
