#include "AiObject.h"

#include "PlayerBotAI.h"
#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "AiObjectContext.h"

AiObject::AiObject(PlayerBotAI* botAI)
    : PlayerbotAIAware(botAI)
{
    bot = botAI ? botAI->me : nullptr;
    chat = nullptr;
}

Player* AiObject::GetMaster()
{
    if (!bot)
        return nullptr;

    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    ObjectGuid leaderGuid = group->GetLeaderGuid();
    if (!leaderGuid.IsEmpty())
    {
        Player* leader = ObjectAccessor::FindPlayer(leaderGuid);
        if (leader && leader->IsPlayer())
            return leader;
    }

    for (GroupReference* gr = group->GetFirstMember(); gr; gr = gr->next())
    {
        Player* member = gr->getSource();
        if (member && member->IsPlayer() && member != bot)
            return member;
    }

    return nullptr;
}

AiObjectContext* AiObject::GetAiObjectContext()
{
    if (!botAI)
        return nullptr;
    return botAI->GetAiObjectContext();
}
