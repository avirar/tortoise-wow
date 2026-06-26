/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#include "AcceptResurrectAction.h"
#include "PlayerBotAI.h"
#include "Player.h"
#include "WorldPacket.h"
#include "Logging.h"
#include "Log.h"

bool AcceptResurrectAction::Execute(Event event)
{
    if (bot->IsAlive())
        return false;

    // For standalone bots, this is rarely useful since no one offers resurrection
    // But handle it if the event has a unit (the resurrector)
    Unit* resurrector = event.GetUnit();
    if (!resurrector)
        return false;

    LOG_DEBUG("playerbots", "%s: accepting resurrection", bot->GetName());

    WorldPacket packet(CMSG_RESURRECT_RESPONSE);
    packet << resurrector->GetGUID();
    packet << uint8(1); // accept
    bot->GetSession()->HandleResurrectResponseOpcode(packet);
    return true;
}
