#include "Queue.h"

#include "Logging.h"

void ActionQueue::Add(NextAction const& action)
{
    if (queue.size() >= maxSize)
    {
        LOG_ERROR("playerbot", "Action queue overflow! Max size: {}", maxSize);
        return;
    }

    queue.push_back(action);
}

void ActionQueue::Clear()
{
    queue.clear();
}
