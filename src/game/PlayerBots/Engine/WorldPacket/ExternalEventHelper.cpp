#include "ExternalEventHelper.h"
#include "WorldPacket.h"
#include "Player.h"
#include "AiObjectContext.h"
#include "Trigger/Trigger.h"

void ExternalEventHelper::HandlePacket(std::map<uint16, std::string> const& handlers, WorldPacket const& packet, Player* owner)
{
    uint16 opcode = packet.GetOpcode();
    auto it = handlers.find(opcode);
    if (it == handlers.end())
        return;

    std::string const& name = it->second;
    Trigger* trigger = aiObjectContext->GetTrigger(name);
    if (!trigger)
        return;

    trigger->ExternalEvent(packet, owner);
}
