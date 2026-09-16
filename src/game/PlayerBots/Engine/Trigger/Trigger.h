#ifndef _PLAYERBOT_TRIGGER_H
#define _PLAYERBOT_TRIGGER_H

#include <string>
#include <vector>
#include <memory>
#include "AiObject.h"
#include "Event.h"
#include "Action/Action.h"

class PlayerBotAI;
class Unit;
class Player;
class WorldPacket;

class Trigger : public AiNamedObject
{
public:
    Trigger(PlayerBotAI* botAI, std::string const& name = "trigger", int32 checkInterval = 1);
    virtual ~Trigger() {}

    virtual Event Check();
    virtual bool IsActive();
    virtual void Reset() {}
    virtual std::vector<NextAction> getHandlers() { return {}; }

    // External event (packet trigger, chat command, etc.)
    virtual void ExternalEvent(WorldPacket const& /*packet*/, Player* /*owner*/) {}
    virtual void ExternalEvent(std::string const& /*param*/, Player* /*owner*/) {}

    Unit* GetTarget();

    bool needCheck(uint32 now);

protected:
    int32 checkInterval;
    uint32 lastCheckTime;
};

#endif
