#include "GroupLeaderValue.h"

#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"

GroupLeaderValue::GroupLeaderValue(PlayerBotAI* botAI)
    : UnitCalculatedValue(botAI, "group leader", 1)
{
}

Unit* GroupLeaderValue::Calculate()
{
    Group* group = bot->GetGroup();
    if (group)
    {
        ObjectGuid leaderGuid = group->GetLeaderGuid();
        if (leaderGuid)
        {
            Unit* leader = ObjectAccessor::GetUnit(*bot, leaderGuid);
            if (leader)
                return leader;
        }
    }

    return bot;
}
