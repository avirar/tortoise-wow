#include "FollowActions.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "Timer.h"

bool FollowAction::Execute(Event /*event*/)
{
    Player* master = GetMaster();
    if (!master || !master->IsInWorld())
        return false;

    if (bot->GetMapId() != master->GetMapId())
        return false;

    float dist = bot->GetDistance(master);
    float followDist = 5.0f;

    if (dist < followDist)
        return false;

    if (bot->GetStandState() != UNIT_STAND_STATE_STAND)
        bot->SetStandState(UNIT_STAND_STATE_STAND);

    if (bot->IsNonMeleeSpellCasted(true))
    {
        bot->CastStop();
        bot->InterruptSpell(CURRENT_GENERIC_SPELL);
    }

    float x = master->GetPositionX();
    float y = master->GetPositionY();
    float z = master->GetPositionZ();

    MotionMaster* mm = bot->GetMotionMaster();
    if (mm)
    {
        mm->MovePoint(0, x, y, z, MOVE_NONE, 0.0f, -10);
        return true;
    }

    return false;
}

bool FollowAction::isUseful()
{
    Player* master = GetMaster();
    if (!master || !master->IsInWorld())
        return false;

    if (bot->GetMapId() != master->GetMapId())
        return false;

    if (master->HasUnitState(UNIT_STAT_TAXI_FLIGHT))
        return false;

    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr)
        return false;

    float dist = bot->GetDistance(master);
    float followDist = 5.0f;

    return dist > followDist;
}

bool FleeToGroupLeaderAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    ObjectGuid leaderGuid = group->GetLeaderGuid();
    if (leaderGuid.IsEmpty())
        return false;

    Player* leader = ObjectAccessor::FindPlayer(leaderGuid);
    if (!leader || !leader->IsInWorld())
        return false;

    if (bot->GetMapId() != leader->GetMapId())
        return false;

    float dist = bot->GetDistance(leader);

    if (dist < 5.0f)
        return false;

    if (bot->GetStandState() != UNIT_STAND_STATE_STAND)
        bot->SetStandState(UNIT_STAND_STATE_STAND);

    MotionMaster* mm = bot->GetMotionMaster();
    if (mm)
    {
        mm->MovePoint(0, leader->GetPositionX(), leader->GetPositionY(), leader->GetPositionZ(),
                      MOVE_NONE, 0.0f, -10);
        return true;
    }

    return false;
}

bool FleeToGroupLeaderAction::isUseful()
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    ObjectGuid leaderGuid = group->GetLeaderGuid();
    if (leaderGuid.IsEmpty())
        return false;

    Player* leader = ObjectAccessor::FindPlayer(leaderGuid);
    if (!leader || !leader->IsInWorld())
        return false;

    if (bot->GetMapId() != leader->GetMapId())
        return false;

    return bot->GetDistance(leader) > 5.0f;
}
