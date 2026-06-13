#ifndef _PLAYERBOT_QUEUE_H
#define _PLAYERBOT_QUEUE_H

#include <vector>
#include <string>
#include "Action/Action.h"

struct NextAction;

class ActionQueue
{
public:
    ActionQueue() : maxSize(500) {}

    void Add(NextAction const& action);
    void Clear();

    std::vector<NextAction> const& GetQueue() const { return queue; }
    bool Empty() const { return queue.empty(); }
    size_t Size() const { return queue.size(); }

    void SetMaxSize(uint32 size) { maxSize = size; }

private:
    std::vector<NextAction> queue;
    uint32 maxSize;
};

#endif
