#ifndef _PLAYERBOT_WORLD_PACKET_TRIGGER_H
#define _PLAYERBOT_WORLD_PACKET_TRIGGER_H

#include "Trigger.h"
#include <memory>

class Player;
class WorldPacket;

class WorldPacketTrigger : public Trigger
{
public:
    WorldPacketTrigger(PlayerBotAI* botAI, std::string const& name)
        : Trigger(botAI, name), m_packet(nullptr), m_owner(nullptr) {}

    void ExternalEvent(WorldPacket const& packet, Player* owner) override;
    Event Check() override;
    bool IsActive() override;

private:
    std::shared_ptr<WorldPacket> m_packet;
    Player* m_owner;
};

#endif
