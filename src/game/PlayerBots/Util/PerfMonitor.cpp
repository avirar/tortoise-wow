#include "PerfMonitor.h"
#include <algorithm>

PerfMonitorOperation::PerfMonitorOperation(PerformanceData* d, std::string const n, PerformanceStack* s)
    : data(d), name(n), stack(s), started(std::chrono::high_resolution_clock::now().time_since_epoch())
{
    if (stack)
        stack->push_back(name);
}

void PerfMonitorOperation::finish()
{
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    uint64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - started).count();

    std::lock_guard<std::mutex> lock(data->lock);
    if (data->count == 0)
    {
        data->minTime = elapsed;
        data->maxTime = elapsed;
        data->totalTime = elapsed;
    }
    else
    {
        data->minTime = std::min(data->minTime, elapsed);
        data->maxTime = std::max(data->maxTime, elapsed);
        data->totalTime += elapsed;
    }
    data->count++;

    if (stack)
        stack->pop_back();
}

PerfMonitorOperation* PerfMonitor::start(PerformanceMetric metric, std::string const name, PerformanceStack* stack)
{
    std::lock_guard<std::mutex> lock(lock);
    auto& metricMap = data[metric];
    if (!metricMap.count(name))
    {
        PerformanceData* pd = new PerformanceData();
        pd->minTime = 0;
        pd->maxTime = 0;
        pd->totalTime = 0;
        pd->count = 0;
        metricMap[name] = pd;
    }
    return new PerfMonitorOperation(metricMap[name], name, stack);
}

void PerfMonitor::PrintStats(bool /*perTick*/, bool /*fullStack*/)
{
}

void PerfMonitor::Reset()
{
    std::lock_guard<std::mutex> lock(lock);
    for (auto& metricPair : data)
    {
        for (auto& namePair : metricPair.second)
        {
            delete namePair.second;
            namePair.second = nullptr;
        }
        metricPair.second.clear();
    }
}
