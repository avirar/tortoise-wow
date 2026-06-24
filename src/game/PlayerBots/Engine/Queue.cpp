#include "Queue.h"

#include "Logging.h"
#include "Timer.h"
#include "PlayerbotAIConfig.h"

void ActionQueue::Push(ActionBasket* action)
{
    if (!action)
        return;

    for (ActionBasket* basket : actions)
    {
        if (basket && action->getAction()->getName() == basket->getAction()->getName())
        {
            updateExistingBasket(basket, action);
            return;
        }
    }

    actions.push_back(action);
}

ActionNode* ActionQueue::Pop()
{
    ActionBasket* highestRelevanceBasket = findHighestRelevanceBasket();
    if (!highestRelevanceBasket)
        return nullptr;

    return extractAndDeleteBasket(highestRelevanceBasket);
}

ActionBasket* ActionQueue::Peek()
{
    return findHighestRelevanceBasket();
}

void ActionQueue::RemoveExpired()
{
    uint32 expiryTime = sPlayerbotAIConfig.expireActionTime;
    if (!expiryTime)
        return;

    for (std::list<ActionBasket*>::iterator it = actions.begin(); it != actions.end(); )
    {
        ActionBasket* basket = *it;
        if (basket && basket->isExpired(expiryTime))
        {
            it = actions.erase(it);
            delete basket;
        }
        else
        {
            ++it;
        }
    }
}

void ActionQueue::Clear()
{
    for (ActionBasket* basket : actions)
    {
        delete basket;
    }
    actions.clear();
}

void ActionQueue::updateExistingBasket(ActionBasket* existing, ActionBasket* newBasket)
{
    if (existing->getRelevance() < newBasket->getRelevance())
    {
        existing->setRelevance(newBasket->getRelevance());
    }

    if (ActionNode* actionNode = newBasket->getAction())
    {
        delete actionNode;
    }

    delete newBasket;
}

ActionBasket* ActionQueue::findHighestRelevanceBasket() const
{
    if (actions.empty())
        return nullptr;

    float maxRelevance = -1.0f;
    ActionBasket* selection = nullptr;

    for (ActionBasket* basket : actions)
    {
        if (!basket)
            continue;

        if (basket->getRelevance() > maxRelevance)
        {
            maxRelevance = basket->getRelevance();
            selection = basket;
        }
    }

    return selection;
}

ActionNode* ActionQueue::extractAndDeleteBasket(ActionBasket* basket)
{
    if (!basket)
        return nullptr;

    ActionNode* action = basket->getAction();
    actions.remove(basket);
    delete basket;
    return action;
}
