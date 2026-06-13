#include "Action.h"

#include "PerfMonitor.h"
#include "Timer.h"

Action::Action(PlayerBotAI* botAI, std::string const& name, int32 repeatInterval)
    : AiNamedObject(botAI, name),
      repeatInterval(repeatInterval == 1 ? 1 : (repeatInterval < 100 ? repeatInterval * 1000 : repeatInterval)),
      lastRepeatTime(0)
{
}

bool Action::Execute([[maybe_unused]] Event const& event)
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
