#include "MasterTargetValue.h"

#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"

MasterTargetValue::MasterTargetValue(PlayerBotAI* botAI)
    : UnitCalculatedValue(botAI, "master target", 1)
{
}

Unit* MasterTargetValue::Calculate()
{
    Player* master = GetMaster();
    if (!master || !master->IsInWorld())
        return nullptr;

    Unit* target = master->GetVictim();
    if (!target)
        target = ObjectAccessor::GetUnit(*master, master->GetSelectionGuid());

    return target;
}
