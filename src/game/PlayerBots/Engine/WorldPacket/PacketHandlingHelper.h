#ifndef _PLAYERBOT_PACKET_HANDLING_HELPER_H
#define _PLAYERBOT_PACKET_HANDLING_HELPER_H

#include <map>
#include <string>
#include <queue>
#include <functional>

class WorldPacket;
class ExternalEventHelper;

class PacketHandlingHelper
{
public:
    void AddHandler(uint16 opcode, std::string const& handler);
    void AddPacket(WorldPacket const& packet);
    void Handle(ExternalEventHelper& helper);

private:
    struct PacketCompare
    {
        bool operator()(WorldPacket const& a, WorldPacket const& b) const
        {
            return a.size() > b.size(); // Min-heap by size
        }
    };

    std::map<uint16, std::string> handlers;
    std::priority_queue<WorldPacket, std::vector<WorldPacket>, PacketCompare> queue;
};

#endif
