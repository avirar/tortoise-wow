#include "WorldPacketTrigger.h"
#include "Player.h"
#include "WorldPacket.h"

void WorldPacketTrigger::ExternalEvent(WorldPacket const& packet, Player* owner)
{
    m_packet = std::make_shared<WorldPacket>(packet);
    m_owner = owner;
    lastCheckTime = 0; // Force Check() to fire on next tick
}

Event WorldPacketTrigger::Check()
{
    if (m_packet)
    {
        Event event(getName(), m_packet);
        m_packet.reset();
        m_owner = nullptr;
        return event;
    }
    return Event();
}

bool WorldPacketTrigger::IsActive()
{
    return m_packet != nullptr;
}
