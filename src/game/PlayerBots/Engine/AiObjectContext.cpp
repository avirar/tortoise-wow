#include "AiObjectContext.h"

#include "Value/Value.h"
#include "Action/Action.h"
#include "Trigger/Trigger.h"
#include "Strategy/Strategy.h"
#include "PerfMonitor.h"
#include "BaseAiObjectContext.h"
#include "Logging.h"
#include "Ai/Base/StrategyContext.h"
#include "WorldPacket.h"

SharedNamedObjectContextList<Action> AiObjectContext::sharedActionContexts;
SharedNamedObjectContextList<Trigger> AiObjectContext::sharedTriggerContexts;
SharedNamedObjectContextList<Strategy> AiObjectContext::sharedStrategyContexts;

AiObjectContext::AiObjectContext()
    : botAI(nullptr),
      actionContexts(sharedActionContexts),
      triggerContexts(sharedTriggerContexts),
      strategyContexts(sharedStrategyContexts)
{
}

void AiObjectContext::BuildAllSharedContexts()
{
    BuildSharedActionContexts(sharedActionContexts);
    BuildSharedTriggerContexts(sharedTriggerContexts);
    // AC pattern: BuildSharedStrategyContexts populates the shared strategy factory
    sharedStrategyContexts.Add(new StrategyContext());
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

Strategy* AiObjectContext::GetStrategy(std::string const& name)
{
    return strategyContexts.GetContextObject(name, botAI);
}

void AiObjectContext::FireTrigger(std::string const& name, std::shared_ptr<WorldPacket> packet)
{
    // AC pattern: packet triggers fire via ExternalEvent on the trigger
    // The Engine will pick up the trigger event on the next ProcessTriggers call
    // For now, we store the packet on the trigger so it can be accessed by actions
    Trigger* trigger = GetTrigger(name);
    if (trigger && botAI && botAI->me)
    {
        trigger->ExternalEvent(*packet, botAI->me);
    }
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
