#include "Action.h"

#include "PerfMonitor.h"
#include "Timer.h"

Action::Action(PlayerBotAI* botAI, std::string const& name)
    : AiNamedObject(botAI, name),
      verbose(false),
      relevance(0.0f),
      repeatInterval(1),
      lastRepeatTime(0)
{
}

bool Action::Execute([[maybe_unused]] Event event)
{
    return false;
}

bool Action::needRepeat(uint32 now)
{
    if (repeatInterval < 2)
        return false;

    if (!lastRepeatTime || now - lastRepeatTime >= repeatInterval)
    {
        lastRepeatTime = now;
        return true;
    }

    return false;
}

ActionBasket::ActionBasket(ActionNode* act, float rel, bool skipPrereq, Event evt)
    : action(act), relevance(rel), skipPrerequisites(skipPrereq), event(evt), created(getMSTime())
{
}

bool ActionBasket::isExpired(uint32_t msecs)
{
    return getMSTime() - created >= msecs;
}
