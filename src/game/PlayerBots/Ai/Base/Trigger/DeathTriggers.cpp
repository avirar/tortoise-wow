/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#include "DeathTriggers.h"
#include "PlayerBotAI.h"
#include "Player.h"
#include "Corpse.h"

bool DeadTrigger::IsActive()
{
    return bot->IsDead();
}

bool CorpseNearTrigger::IsActive()
{
    Corpse* corpse = bot->GetCorpse();
    return corpse && corpse->IsWithinDistInMap(bot, CORPSE_RECLAIM_RADIUS, true);
}

bool ResurrectRequestTrigger::IsActive()
{
    // For standalone bots, always false (no group members to resurrect us)
    return false;
}

bool CanSelfResurrectTrigger::IsActive()
{
    // Check for shaman Reincarnation (spell 20083) or warlock Soulstone
    // For standalone bots, this is rarely useful
    return bot->IsDead() && (bot->HasAura(20083) || bot->HasAura(20566));
}
