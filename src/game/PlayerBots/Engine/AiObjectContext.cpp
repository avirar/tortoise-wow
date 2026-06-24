#include "AiObjectContext.h"

#include "Value/Value.h"
#include "Action/Action.h"
#include "Trigger/Trigger.h"
#include "PerfMonitor.h"
#include "BaseAiObjectContext.h"
#include "Logging.h"

SharedNamedObjectContextList<Action> AiObjectContext::sharedActionContexts;
SharedNamedObjectContextList<Trigger> AiObjectContext::sharedTriggerContexts;

AiObjectContext::AiObjectContext()
    : botAI(nullptr),
      actionContexts(sharedActionContexts),
      triggerContexts(sharedTriggerContexts)
{
}

void AiObjectContext::BuildAllSharedContexts()
{
    BuildSharedActionContexts(sharedActionContexts);
    BuildSharedTriggerContexts(sharedTriggerContexts);
}

void AiObjectContext::Init(PlayerBotAI* botAI)
{
    this->botAI = botAI;

    static bool built = false;
    if (!built)
    {
        BuildAllSharedContexts();
        built = true;
    }

    BuildSharedBaseAiObjectContext(botAI);
}

void AiObjectContext::Reset()
{
    for (std::map<std::string, UntypedValue*>::iterator i = values.begin(); i != values.end(); ++i)
    {
        delete i->second;
    }
    values.clear();
    performanceStack.clear();
}

UntypedValue* AiObjectContext::GetUntypedValue(std::string const& name)
{
    std::map<std::string, UntypedValue*>::iterator it = values.find(name);
    if (it != values.end())
        return it->second;
    return nullptr;
}

Action* AiObjectContext::GetAction(std::string const& name)
{
    return actionContexts.GetContextObject(name, botAI);
}

Trigger* AiObjectContext::GetTrigger(std::string const& name)
{
    return triggerContexts.GetContextObject(name, botAI);
}

std::string const AiObjectContext::Format()
{
    std::ostringstream out;
    out << "{";
    for (std::map<std::string, UntypedValue*>::iterator i = values.begin(); i != values.end(); ++i)
    {
        out << i->first << ":" << i->second->Format() << ",";
    }
    out << "}";
    return out.str();
}
