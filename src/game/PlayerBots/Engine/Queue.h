#ifndef _PLAYERBOT_QUEUE_H
#define _PLAYERBOT_QUEUE_H

#include <list>
#include <string>
#include "Action/Action.h"

class ActionBasket;

class ActionQueue
{
public:
    ActionQueue() = default;
    ~ActionQueue() { Clear(); }

    void Push(ActionBasket* action);
    ActionNode* Pop();
    ActionBasket* Peek();
    uint32 Size() { return actions.size(); }
    void RemoveExpired();
    void Clear();

    bool Empty() const { return actions.empty(); }

private:
    void updateExistingBasket(ActionBasket* existing, ActionBasket* newBasket);
    ActionBasket* findHighestRelevanceBasket() const;
    ActionNode* extractAndDeleteBasket(ActionBasket* basket);

    std::list<ActionBasket*> actions;
};

#endif
