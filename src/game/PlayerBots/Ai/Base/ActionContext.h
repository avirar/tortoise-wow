#ifndef PLAYERBOT_BASE_ACTION_CONTEXT_H
#define PLAYERBOT_BASE_ACTION_CONTEXT_H

#include "NamedObjectContext.h"
#include "Action/Action.h"

#include "Actions/FollowActions.h"
#include "Actions/AttackAction.h"
#include "Actions/ChooseTargetActions.h"
#include "Actions/ReachTargetActions.h"
#include "Actions/CombatActions.h"
#include "Actions/NonCombatActions.h"
#include "Actions/MovementActions.h"

class PlayerBotAI;

void BuildSharedActionContexts(SharedNamedObjectContextList<Action>& actionContexts);

#endif
