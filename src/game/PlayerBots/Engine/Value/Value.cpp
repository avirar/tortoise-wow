#include "Value.h"

#include "PerfMonitor.h"
#include "Timer.h"
#include "AiObjectContext.h"

std::string const Uint8CalculatedValue::Format()
{
    std::ostringstream out;
    out << Calculate();
    return out.str();
}

std::string const Uint32CalculatedValue::Format()
{
    std::ostringstream out;
    out << Calculate();
    return out.str();
}

std::string const FloatCalculatedValue::Format()
{
    std::ostringstream out;
    out << Calculate();
    return out.str();
}

UnitCalculatedValue::UnitCalculatedValue(PlayerBotAI* botAI, std::string const& name, int32 checkInterval)
    : CalculatedValue<Unit*>(botAI, name, checkInterval)
{
}

std::string const UnitCalculatedValue::Format()
{
    Unit* unit = Calculate();
    return unit ? unit->GetName() : "<none>";
}

std::string const UnitManualSetValue::Format()
{
    Unit* unit = Get();
    return unit ? unit->GetName() : "<none>";
}

Unit* UnitCalculatedValue::Get()
{
    if (checkInterval < 2)
    {
        PerfMonitorOperation* pmo = sPerfMonitor.start(
            PERF_MON_VALUE, this->getName(), this->context ? &this->context->performanceStack : nullptr);
        value = Calculate();
        if (pmo)
            pmo->finish();
    }
    else
    {
        time_t now = getMSTime();
        if (!lastCheckTime || now - lastCheckTime >= checkInterval)
        {
            lastCheckTime = now;
            PerfMonitorOperation* pmo = sPerfMonitor.start(
                PERF_MON_VALUE, this->getName(), this->context ? &this->context->performanceStack : nullptr);
            value = Calculate();
            if (pmo)
                pmo->finish();
        }
    }
    if (value && value->IsInWorld())
        return value;
    return nullptr;
}

Unit* UnitManualSetValue::Get()
{
    if (value && value->IsInWorld())
        return value;
    return nullptr;
}
