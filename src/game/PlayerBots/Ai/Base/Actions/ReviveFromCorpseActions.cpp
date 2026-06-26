/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#include "ReviveFromCorpseActions.h"
#include "PlayerBotAI.h"
#include "Player.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "Corpse.h"
#include "MotionMaster.h"
#include "Logging.h"
#include "Log.h"

// FindCorpseAction - move toward corpse
bool FindCorpseAction::Execute(Event event)
{
    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
        return false;

    float dist = bot->GetDistance(corpse);
    if (dist < CORPSE_RECLAIM_RADIUS - 5.0f)
        return false; // Already near corpse, let ReviveFromCorpseAction handle it

    LOG_DEBUG("playerbots", "%s: moving to corpse (dist=%.1f)", bot->GetName(), dist);

    bot->GetMotionMaster()->Clear();
    bot->StopMoving();
    bot->GetMotionMaster()->MovePoint(1, corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ());
    return true;
}

bool FindCorpseAction::isUseful()
{
    return bot->IsDead() && bot->GetCorpse();
}

// ReviveFromCorpseAction - reclaim corpse when near
bool ReviveFromCorpseAction::Execute(Event event)
{
    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
        return false;

    if (!corpse->IsWithinDistInMap(bot, CORPSE_RECLAIM_RADIUS, true))
        return false; // Too far, let FindCorpseAction handle it

    LOG_DEBUG("playerbots", "%s: reviving from corpse", bot->GetName());

    bot->GetMotionMaster()->Clear();
    bot->StopMoving();

    WorldPacket packet(CMSG_RECLAIM_CORPSE);
    packet << bot->GetGUID();
    bot->GetSession()->HandleReclaimCorpseOpcode(packet);
    return true;
}

// SpiritHealerAction - teleport to graveyard and find spirit healer
bool SpiritHealerAction::Execute(Event event)
{
    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
        return false;

    WorldSafeLocsEntry const* graveyard = sObjectMgr.GetClosestGraveYard(
        bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
        bot->GetMapId(), bot->GetTeam());

    if (!graveyard)
        return false;

    LOG_DEBUG("playerbots", "%s: teleporting to graveyard for spirit healer (%.1f, %.1f)",
              bot->GetName(), graveyard->x, graveyard->y);

    bot->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TELEPORTED | AURA_INTERRUPT_FLAG_CHANGE_MAP);
    bot->TeleportTo(graveyard->map_id, graveyard->x, graveyard->y, graveyard->z, 0.0f);
    return true;
}

bool SpiritHealerAction::isUseful()
{
    return bot->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
}
