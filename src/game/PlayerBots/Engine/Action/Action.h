#ifndef _PLAYERBOT_ACTION_H
#define _PLAYERBOT_ACTION_H

#include <vector>
#include <string>
#include "AiObject.h"
#include "Event.h"
#include "Timer.h"

class PlayerBotAI;
class Unit;

struct NextAction
{
    std::string actionName;
    float priority;
    std::string eventHint;

    NextAction(std::string const& name, float prio = 5.0f, std::string const& hint = "")
        : actionName(name), priority(prio), eventHint(hint) {}
};

class Action : public AiNamedObject
{
public:
    Action(PlayerBotAI* botAI, std::string const& name = "action", int32 repeatInterval = 1);
    virtual ~Action() {}

    virtual bool Execute(Event const& event);

    virtual std::string const GetHint() { return "execute"; }
    virtual std::string const GetEvent() { return "execute"; }

    bool needRepeat(uint32 now);

protected:
    int32 repeatInterval;
    uint32 lastRepeatTime;
};

class ActionNode : public AiNamedObject
{
public:
    ActionNode(std::string const& name,
               std::vector<NextAction> const& P,
               std::vector<NextAction> const& A,
               std::vector<NextAction> const& C)
        : AiNamedObject(nullptr, name),
          P(P), A(A), C(C) {}

    virtual ~ActionNode() {}

    std::vector<NextAction> const& GetP() const { return P; }
    std::vector<NextAction> const& GetA() const { return A; }
    std::vector<NextAction> const& GetC() const { return C; }

private:
    std::vector<NextAction> P;
    std::vector<NextAction> A;
    std::vector<NextAction> C;
};

#endif
