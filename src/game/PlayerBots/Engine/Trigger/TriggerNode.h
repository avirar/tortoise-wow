#ifndef _PLAYERBOT_TRIGGER_NODE_H
#define _PLAYERBOT_TRIGGER_NODE_H

#include <vector>
#include <string>
#include "Action/Action.h"

class Trigger;

class TriggerNode
{
public:
    TriggerNode(
        std::string const& name,
        std::vector<NextAction> handlers = {}
    ) : trigger(nullptr), handlers(handlers), name(name)
    {}

    virtual ~TriggerNode() {}

    Trigger* getTrigger() { return trigger; }
    void setTrigger(Trigger* trg) { this->trigger = trg; }
    std::string const getName() { return name; }

    std::vector<NextAction> getHandlers()
    {
        std::vector<NextAction> result = this->handlers;

        if (trigger != nullptr)
        {
            std::vector<NextAction> extra = trigger->getHandlers();
            result.insert(result.end(), extra.begin(), extra.end());
        }

        return result;
    }

    float getFirstRelevance()
    {
        if (this->handlers.size() > 0)
            return this->handlers[0].getRelevance();

        return -1;
    }

private:
    Trigger* trigger;
    std::vector<NextAction> handlers;
    std::string name;
};

#endif
