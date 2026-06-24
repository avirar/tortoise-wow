#ifndef PLAYERBOT_BASE_AI_OBJECT_CONTEXT_H
#define PLAYERBOT_BASE_AI_OBJECT_CONTEXT_H

#include "NamedObjectContext.h"
#include "Action/Action.h"
#include "Trigger/Trigger.h"

class PlayerBotAI;

void BuildSharedBaseAiObjectContext(PlayerBotAI* botAI);
void BuildSharedActionContexts(SharedNamedObjectContextList<Action>& actionContexts);
void BuildSharedTriggerContexts(SharedNamedObjectContextList<Trigger>& triggerContexts);

#endif
