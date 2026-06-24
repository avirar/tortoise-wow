#ifndef PLAYERBOT_BASE_TRIGGER_CONTEXT_H
#define PLAYERBOT_BASE_TRIGGER_CONTEXT_H

#include "NamedObjectContext.h"
#include "Trigger/Trigger.h"

#include "Trigger/CombatTrigger.h"
#include "Trigger/HealthTrigger.h"
#include "Trigger/CombatTrigger.h"

class PlayerBotAI;

void BuildSharedTriggerContexts(SharedNamedObjectContextList<Trigger>& triggerContexts);

#endif
