#ifndef PLAYERBOT_GROUP_LEADER_VALUE_H
#define PLAYERBOT_GROUP_LEADER_VALUE_H

#include "Value/Value.h"

class PlayerBotAI;

class GroupLeaderValue : public UnitCalculatedValue
{
public:
    GroupLeaderValue(PlayerBotAI* botAI);
    Unit* Calculate() override;
};

#endif
