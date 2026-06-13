#ifndef _PLAYERBOT_TRIGGER_H
#define _PLAYERBOT_TRIGGER_H

#include <string>
#include "AiObject.h"
#include "Event.h"

class PlayerBotAI;
class Unit;

class Trigger : public AiNamedObject
{
public:
    Trigger(PlayerBotAI* botAI, std::string const& name = "trigger", int32 checkInterval = 1);
    virtual ~Trigger() {}

    virtual Event Check();
    virtual bool IsActive();

    Unit* GetTarget();

    bool needCheck(uint32 now);

protected:
    int32 checkInterval;
    uint32 lastCheckTime;
};

#endif
