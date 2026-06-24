#include "ServerFacade.h"
#include "Player.h"
#include "Unit.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"

float ServerFacade::GetDistance2d(Unit* unit, WorldObject* wo)
{
    if (!unit || !wo)
        return 0.f;

    return unit->GetDistance2d(wo);
}

float ServerFacade::GetDistance2d(Unit* unit, float x, float y)
{
    if (!unit)
        return 0.f;

    return unit->GetDistance2d(x, y);
}

bool ServerFacade::IsDistanceLessThan(float dist1, float dist2)
{
    return dist1 < dist2;
}

bool ServerFacade::IsDistanceGreaterThan(float dist1, float dist2)
{
    return dist1 > dist2;
}

bool ServerFacade::IsDistanceGreaterOrEqualThan(float dist1, float dist2)
{
    return !IsDistanceLessThan(dist1, dist2);
}

bool ServerFacade::IsDistanceLessOrEqualThan(float dist1, float dist2)
{
    return !IsDistanceGreaterThan(dist1, dist2);
}

void ServerFacade::SetFacingTo(Player* bot, WorldObject* wo, bool /*force*/)
{
    if (!bot || !wo)
        return;

    float angle = bot->GetAngle(wo);
    bot->SetOrientation(angle);
}

Unit* ServerFacade::GetChaseTarget(Unit* target)
{
    if (!target)
        return nullptr;

    MovementGenerator* movementGen = target->GetMotionMaster()->top();
    if (movementGen && movementGen->GetMovementGeneratorType() == CHASE_MOTION_TYPE)
    {
        return target->GetVictim();
    }

    return nullptr;
}

void ServerFacade::SendPacket(Player* player, WorldPacket* packet)
{
    if (player && packet && player->GetSession())
        player->GetSession()->SendPacket(packet);
}

Unit* ServerFacade::SelectNearestHostileTarget(Unit* unit, float range)
{
    if (!unit || !unit->IsAlive())
        return nullptr;

    return unit->SelectNearestUnfriendlyTarget(range);
}
