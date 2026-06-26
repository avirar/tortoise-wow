#include "PerfMonitor.h"
#include "Logging.h"
#include "Log.h"
#include <algorithm>

PerfMonitorOperation::PerfMonitorOperation(PerformanceData* d, std::string const n, PerformanceStack* s)
    : data(d), name(n), stack(s), started(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()))
{
    if (stack)
        stack->push_back(name);
}

void PerfMonitorOperation::finish()
{
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
    uint64_t elapsed = uint64_t(now.count() - started.count());

    std::lock_guard<std::mutex> lg(data->lock);
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
    std::lock_guard<std::mutex> lg(lock);
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

void PerfMonitor::PrintStats(bool perTick, bool fullStack)
{
    std::lock_guard<std::mutex> lg(lock);
    if (data.empty())
    {
        LOG_DEBUG("playerbots", "[PerfMonitor] No data collected (enable with 'playerbots pmon toggle')");
        return;
    }

    // Find total UpdateAI time for percentage calculation
    uint64_t updateAITotalTime = 0;
    auto& totalMap = data[PERF_MON_TOTAL];
    for (auto& pair : totalMap)
    {
        if (pair.first.find("PlayerbotAI::UpdateAI") != std::string::npos)
            updateAITotalTime += pair.second->totalTime;
    }

    if (perTick)
    {
        // Per-tick mode: normalize by total tick count
        uint32_t fullTickCount = 0;
        uint64_t fullTickTotalTime = 0;
        auto it = totalMap.find("PlayerbotAIBase::FullTick");
        if (it != totalMap.end())
        {
            fullTickCount = it->second->count;
            fullTickTotalTime = it->second->totalTime;
        }

        if (fullTickCount == 0)
            fullTickCount = 1; // prevent div by zero

        LOG_DEBUG("playerbots", "---------------------------------------[PER TICK]------------------------------------------------------");
        LOG_DEBUG("playerbots", "percentage     time  |     min ..     max (      avg  of      count) - type      : name");
        LOG_DEBUG("playerbots", "-------------------------------------------------------------------------------------------------------");

        for (auto& metricPair : data)
        {
            const char* key = "?";
            switch (metricPair.first)
            {
                case PERF_MON_TRIGGER: key = "Trigger"; break;
                case PERF_MON_VALUE:   key = "Value";   break;
                case PERF_MON_ACTION:  key = "Action";  break;
                case PERF_MON_RNDBOT:  key = "RndBot";  break;
                case PERF_MON_TOTAL:   key = "Total";   break;
                default: break;
            }

            uint64_t typeTotalTime = 0;
            uint64_t typeMinTime = 0xFFFFFFFFu;
            uint64_t typeMaxTime = 0;
            uint32_t typeCount = 0;

            for (auto& namePair : metricPair.second)
            {
                PerformanceData* pd = namePair.second;
                typeTotalTime += pd->totalTime;
                typeCount += pd->count;
                if (pd->minTime < typeMinTime) typeMinTime = pd->minTime;
                if (pd->maxTime > typeMaxTime) typeMaxTime = pd->maxTime;

                float perc = (float)pd->totalTime / fullTickCount / 1000.0f;
                float avg = (float)pd->totalTime / (float)pd->count / 1000.0f;
                float amount = (float)pd->count / fullTickCount;
                std::string disName = namePair.first;
                if (!fullStack && disName.find("|") != std::string::npos)
                    disName = disName.substr(0, disName.find("|")) + "]";

                if (perc >= 0.1f || avg >= 0.25f || pd->maxTime > 1000)
                {
                    LOG_DEBUG("playerbots", "  %7.3f%% %9.3fms | %7.1f .. %7.1f (%10.3f of %10.2f) - %-6s    : %s",
                              perc, perc, (float)pd->minTime/1000.0f, (float)pd->maxTime/1000.0f,
                              avg, amount, key, disName.c_str());
                }
            }

            if (metricPair.first != PERF_MON_TOTAL && typeCount > 0)
            {
                float tPerc = (float)typeTotalTime / fullTickCount / 1000.0f;
                float tAvg = (float)typeTotalTime / (float)typeCount / 1000.0f;
                float tAmount = (float)typeCount / fullTickCount;
                LOG_DEBUG("playerbots", "  %7.3f%% %9.3fms | %7.1f .. %7.1f (%10.3f of %10.2f) - %-6s    : %s",
                          tPerc, tPerc, (float)typeMinTime/1000.0f, (float)typeMaxTime/1000.0f,
                          tAvg, tAmount, key, "Total");
            }
            LOG_DEBUG("playerbots", " ");
        }
    }
    else
    {
        // Total mode: percentage of total UpdateAI time
        if (updateAITotalTime == 0)
            updateAITotalTime = 1; // prevent div by zero

        LOG_DEBUG("playerbots", "--------------------------------------[TOTAL BOT]------------------------------------------------------");
        LOG_DEBUG("playerbots", "percentage     time  |     min ..     max (      avg  of      count) - type      : name");
        LOG_DEBUG("playerbots", "-------------------------------------------------------------------------------------------------------");

        for (auto& metricPair : data)
        {
            const char* key = "?";
            switch (metricPair.first)
            {
                case PERF_MON_TRIGGER: key = "Trigger"; break;
                case PERF_MON_VALUE:   key = "Value";   break;
                case PERF_MON_ACTION:  key = "Action";  break;
                case PERF_MON_RNDBOT:  key = "RndBot";  break;
                case PERF_MON_TOTAL:   key = "Total";   break;
                default: break;
            }

            uint64_t typeTotalTime = 0;
            uint64_t typeMinTime = 0xFFFFFFFFu;
            uint64_t typeMaxTime = 0;
            uint32_t typeCount = 0;

            for (auto& namePair : metricPair.second)
            {
                // In total mode, skip non-UpdateAI entries in Total category
                if (metricPair.first == PERF_MON_TOTAL && namePair.first.find("PlayerbotAI::UpdateAI") == std::string::npos)
                    continue;

                PerformanceData* pd = namePair.second;
                typeTotalTime += pd->totalTime;
                typeCount += pd->count;
                if (pd->minTime < typeMinTime) typeMinTime = pd->minTime;
                if (pd->maxTime > typeMaxTime) typeMaxTime = pd->maxTime;

                float perc = (float)pd->totalTime / updateAITotalTime * 100.0f;
                float time = (float)pd->totalTime / 1000000.0f;
                float avg = (float)pd->totalTime / (float)pd->count / 1000.0f;
                std::string disName = namePair.first;
                if (!fullStack && disName.find("|") != std::string::npos)
                    disName = disName.substr(0, disName.find("|")) + "]";

                if (perc >= 0.1f || avg >= 0.25f || pd->maxTime > 1000)
                {
                    LOG_DEBUG("playerbots", "  %7.3f%% %10.3fs | %7.1f .. %7.1f (%10.3f of %10d) - %-6s    : %s",
                              perc, time, (float)pd->minTime/1000.0f, (float)pd->maxTime/1000.0f,
                              avg, pd->count, key, disName.c_str());
                }
            }

            if (typeCount > 0)
            {
                float tPerc = (float)typeTotalTime / (float)updateAITotalTime * 100.0f;
                float tTime = (float)typeTotalTime / 1000000.0f;
                float tAvg = (float)typeTotalTime / (float)typeCount / 1000.0f;
                LOG_DEBUG("playerbots", "  %7.3f%% %10.3fs | %7.1f .. %7.1f (%10.3f of %10d) - %-6s    : %s",
                          tPerc, tTime, (float)typeMinTime/1000.0f, (float)typeMaxTime/1000.0f,
                          tAvg, typeCount, key, "Total");
            }
            LOG_DEBUG("playerbots", " ");
        }
    }
}

void PerfMonitor::Reset()
{
    std::lock_guard<std::mutex> lg(lock);
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
