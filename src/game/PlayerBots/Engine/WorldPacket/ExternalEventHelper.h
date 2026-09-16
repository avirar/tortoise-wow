#ifndef _PLAYERBOT_EXTERNAL_EVENT_HELPER_H
#define _PLAYERBOT_EXTERNAL_EVENT_HELPER_H

#include <map>
#include <string>

class AiObjectContext;
class Player;
class WorldPacket;

class ExternalEventHelper
{
public:
    ExternalEventHelper(AiObjectContext* aiObjectContext) : aiObjectContext(aiObjectContext) {}

    void HandlePacket(std::map<uint16, std::string> const& handlers, WorldPacket const& packet, Player* owner = nullptr);

private:
    AiObjectContext* aiObjectContext;
};

#endif
