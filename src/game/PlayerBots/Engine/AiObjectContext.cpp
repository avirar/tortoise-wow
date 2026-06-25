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
    LOG_DEBUG("playerbots", "[3ENGINE] AiObjectContext::Init() START, botAI=%p", (void*)botAI);
    this->botAI = botAI;

    static bool built = false;
    if (!built)
    {
        LOG_DEBUG("playerbots", "[3ENGINE] AiObjectContext::Init() calling BuildAllSharedContexts()");
        BuildAllSharedContexts();
        LOG_DEBUG("playerbots", "[3ENGINE] AiObjectContext::Init() BuildAllSharedContexts() done");
        built = true;
    }

    LOG_DEBUG("playerbots", "[3ENGINE] AiObjectContext::Init() calling BuildSharedBaseAiObjectContext()");
    BuildSharedBaseAiObjectContext(botAI, this);
    LOG_DEBUG("playerbots", "[3ENGINE] AiObjectContext::Init() DONE");
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
