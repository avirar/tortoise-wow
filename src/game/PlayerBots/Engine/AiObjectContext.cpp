#include "AiObjectContext.h"

#include "Value/Value.h"
#include "PerfMonitor.h"

AiObjectContext::AiObjectContext()
{
}

void AiObjectContext::Init([[maybe_unused]] PlayerBotAI* botAI)
{
}

void AiObjectContext::Reset()
{
    values.clear();
    performanceStack.clear();
}

std::string const AiObjectContext::Format()
{
    std::ostringstream out;
    out << "{";
    for (std::map<std::string, void*>::iterator i = values.begin(); i != values.end(); ++i)
    {
        out << i->first << ":?,";
    }
    out << "}";
    return out.str();
}
