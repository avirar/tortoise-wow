/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#include "ReleaseSpiritActions.h"
#include "PlayerBotAI.h"
#include "Player.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "Corpse.h"
#include "Logging.h"
#include "Log.h"

// ReleaseSpiritAction
bool ReleaseSpiritAction::Execute(Event event)
{
    if (bot->IsAlive())
        return false;

    if (bot->GetCorpse() && bot->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        return false;

    LOG_DEBUG("playerbots", "%s: releasing spirit", bot->GetName());
    bot->DurabilityRepairAll(false, 1.0f);

    WorldPacket packet(CMSG_REPOP_REQUEST);
    packet << uint8(0);
    bot->GetSession()->HandleRepopRequestOpcode(packet);
    botAI->SetNextCheckDelay(1000);
    return true;
}

// AutoReleaseSpiritAction
bool AutoReleaseSpiritAction::Execute(Event event)
{
    LOG_DEBUG("playerbots", "%s: auto releasing spirit", bot->GetName());
    bot->DurabilityRepairAll(false, 1.0f);

    WorldPacket packet(CMSG_REPOP_REQUEST);
    packet << uint8(0);
    bot->GetSession()->HandleRepopRequestOpcode(packet);
    botAI->SetNextCheckDelay(1000);
    return true;
}

bool AutoReleaseSpiritAction::isUseful()
{
    if (!bot->IsDead())
        return false;

    if (bot->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        return false;

    return true;
}

// RepopAction - teleport to graveyard when stuck
bool RepopAction::Execute(Event event)
{
    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
        return false;

    // Find closest graveyard to bot position
    WorldSafeLocsEntry const* graveyard = sObjectMgr.GetClosestGraveYard(
        bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
        bot->GetMapId(), bot->GetTeam());

    if (!graveyard)
        return false;

    LOG_DEBUG("playerbots", "%s: teleporting to graveyard (%.1f, %.1f)",
              bot->GetName(), graveyard->x, graveyard->y);

    bot->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED | AURA_INTERRUPT_FLAG_CHANGE_MAP);
    bot->TeleportTo(graveyard->map_id, graveyard->x, graveyard->y, graveyard->z, 0.0f);
    return true;
}

bool RepopAction::isUseful()
{
    return bot->IsDead() && !bot->IsAlive();
}

// SelfResurrectAction
bool SelfResurrectAction::Execute(Event event)
{
    if (!bot->IsDead())
        return false;

    LOG_DEBUG("playerbots", "%s: self resurrecting", bot->GetName());
    WorldPacket packet(CMSG_SELF_RES);
    bot->GetSession()->HandleSelfResOpcode(packet);
    return true;
}

bool SelfResurrectAction::isUseful()
{
    // Only useful if bot has a self-resurrection spell active
    // For now, just check if dead and not a ghost
    return bot->IsDead() && !bot->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
}
