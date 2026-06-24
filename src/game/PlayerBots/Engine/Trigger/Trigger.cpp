#include "Trigger.h"

#include "Event.h"
#include "Action/Action.h"

Trigger::Trigger(PlayerBotAI* botAI, std::string const& name, int32 checkInterval)
    : AiNamedObject(botAI, name),
      checkInterval(checkInterval == 1 ? 1 : (checkInterval < 100 ? checkInterval * 1000 : checkInterval)),
      lastCheckTime(0)
{
}

Event Trigger::Check()
{
    if (IsActive())
    {
        Event event(getName());
        return event;
    }

    Event event;
    return event;
}

bool Trigger::IsActive()
{
    return false;
}

Unit* Trigger::GetTarget() { return nullptr; }

bool Trigger::needCheck(uint32 now)
{
    if (checkInterval < 2)
        return true;

    if (!lastCheckTime || now - lastCheckTime >= checkInterval)
    {
        lastCheckTime = now;
        return true;
    }

    return false;
}
