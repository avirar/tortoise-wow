/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Adapted for Tortoise WoW
 */

#include "PerfMonitor.h"

#include "PlayerbotAIConfig.h"
#include "Logging.h"
#include "Log.h"
#include <algorithm>
#include <sstream>

PerfMonitorOperation::PerfMonitorOperation(PerformanceData* d, std::string const n, PerformanceStack* s)
    : data(d), name(n), stack(s)
{
    started = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch();
}

void PerfMonitorOperation::finish()
{
    std::chrono::microseconds finished =
        std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch();
    uint64_t elapsed = (finished - started).count();

    std::lock_guard<std::mutex> guard(data->lock);
    if (elapsed > 0)
    {
        if (!data->minTime || data->minTime > elapsed)
            data->minTime = elapsed;

        if (!data->maxTime || data->maxTime < elapsed)
            data->maxTime = elapsed;

        data->totalTime += elapsed;
    }

    ++data->count;

    if (stack)
    {
        stack->erase(std::remove(stack->begin(), stack->end(), name), stack->end());
    }

    delete this;
}

PerfMonitorOperation* PerfMonitor::start(PerformanceMetric metric, std::string const name, PerformanceStack* stack)
{
    if (!sPlayerbotAIConfig.perfMonEnabled)
        return nullptr;

    std::string stackName = name;

    if (stack)
    {
        if (!stack->empty())
        {
            std::ostringstream out;
            out << stackName << " [";

            for (std::vector<std::string>::reverse_iterator i = stack->rbegin(); i != stack->rend(); ++i)
                out << *i << (std::next(i) == stack->rend() ? "" : "|");

            out << "]";
            stackName = out.str();
        }

        stack->push_back(name);
    }

    std::lock_guard<std::mutex> guard(lock);
    PerformanceData* pd = data[metric][stackName];
    if (!pd)
    {
        pd = new PerformanceData();
        pd->minTime = 0;
        pd->maxTime = 0;
        pd->totalTime = 0;
        pd->count = 0;
        data[metric][stackName] = pd;
    }

    return new PerfMonitorOperation(pd, name, stack);
}

void PerfMonitor::PrintStats(bool perTick, bool fullStack)
{
    if (data.empty())
    {
        sLog.outString("playerbots: [PerfMonitor] No data collected (enable with 'playerbots pmon toggle')");
        return;
    }

    if (!perTick)
    {
        // TOTAL mode: percentage of total FullTick time
        uint64_t updateAITotalTime = 0;
        auto it = data.find(PERF_MON_TOTAL);
        if (it != data.end())
        {
            auto fit = it->second.find("PlayerbotAIBase::FullTick");
            if (fit != it->second.end())
                updateAITotalTime = fit->second->totalTime;
        }

        if (updateAITotalTime == 0)
            updateAITotalTime = 1; // prevent div by zero

        sLog.outString("playerbots: --------------------------------------[TOTAL BOT]------------------------------------------------------");
        sLog.outString("playerbots: percentage     time  |     min ..     max (      avg  of      count) - type      : name");
        sLog.outString("playerbots: -------------------------------------------------------------------------------------------------------");

        for (std::map<PerformanceMetric, std::map<std::string, PerformanceData*> >::iterator i = data.begin();
             i != data.end(); ++i)
        {
            std::map<std::string, PerformanceData*> pdMap = i->second;

            const char* key = "?";
            switch (i->first)
            {
                case PERF_MON_TRIGGER: key = "Trigger"; break;
                case PERF_MON_VALUE:   key = "Value";   break;
                case PERF_MON_ACTION:  key = "Action";  break;
                case PERF_MON_RNDBOT:  key = "RndBot";  break;
                case PERF_MON_TOTAL:   key = "Total";   break;
                default: break;
            }

            std::vector<std::string> names;
            for (std::map<std::string, PerformanceData*>::iterator j = pdMap.begin(); j != pdMap.end(); ++j)
            {
                if (std::string(key) == "Total" && j->first.find("PlayerbotAIBase::FullTick") == std::string::npos)
                    continue;
                names.push_back(j->first);
            }

            std::sort(names.begin(), names.end(),
                      [&pdMap](std::string const& a, std::string const& b)
                      { return pdMap.at(a)->totalTime < pdMap.at(b)->totalTime; });

            uint64_t typeTotalTime = 0;
            uint64_t typeMinTime = 0xFFFFFFFFu;
            uint64_t typeMaxTime = 0;
            uint32_t typeCount = 0;

            for (std::vector<std::string>::iterator k = names.begin(); k != names.end(); ++k)
            {
                PerformanceData* pd = pdMap[*k];
                typeTotalTime += pd->totalTime;
                typeCount += pd->count;
                if (typeMinTime > pd->minTime)
                    typeMinTime = pd->minTime;
                if (typeMaxTime < pd->maxTime)
                    typeMaxTime = pd->maxTime;

                float perc = (float)pd->totalTime / (float)updateAITotalTime * 100.0f;
                float time = (float)pd->totalTime / 1000000.0f;
                float minTime = (float)pd->minTime / 1000.0f;
                float maxTime = (float)pd->maxTime / 1000.0f;
                float avg = (float)pd->totalTime / (float)pd->count / 1000.0f;

                std::string disName = *k;
                if (!fullStack && disName.find("|") != std::string::npos)
                    disName = disName.substr(0, disName.find("|")) + "]";

                if (perc >= 0.1f || avg >= 0.25f || pd->maxTime > 1000)
                {
                    sLog.outString("playerbots: %7.3f%% %10.3fs | %7.1f .. %7.1f (%10.3f of %10d) - %-6s    : %s",
                                   perc, time, minTime, maxTime, avg, pd->count, key, disName.c_str());
                }
            }

            if (typeCount > 0)
            {
                float tPerc = (float)typeTotalTime / (float)updateAITotalTime * 100.0f;
                float tTime = (float)typeTotalTime / 1000000.0f;
                float tMinTime = (float)typeMinTime / 1000.0f;
                float tMaxTime = (float)typeMaxTime / 1000.0f;
                float tAvg = (float)typeTotalTime / (float)typeCount / 1000.0f;
                sLog.outString("playerbots: %7.3f%% %10.3fs | %7.1f .. %7.1f (%10.3f of %10d) - %-6s    : %s",
                               tPerc, tTime, tMinTime, tMaxTime, tAvg, typeCount, key, "Total");
            }
            sLog.outString("playerbots:  ");
        }
    }
    else
    {
        // PER TICK mode: normalize by FullTick count
        auto it = data.find(PERF_MON_TOTAL);
        float fullTickCount = 0;
        uint64_t fullTickTotalTime = 0;
        if (it != data.end())
        {
            auto fit = it->second.find("PlayerbotAIBase::FullTick");
            if (fit != it->second.end())
            {
                fullTickCount = (float)fit->second->count;
                fullTickTotalTime = fit->second->totalTime;
            }
        }

        if (fullTickCount == 0)
            fullTickCount = 1; // prevent div by zero
        if (fullTickTotalTime == 0)
            fullTickTotalTime = 1;

        sLog.outString("playerbots: ---------------------------------------[PER TICK]------------------------------------------------------");
        sLog.outString("playerbots: percentage     time  |     min ..     max (      avg  of      count) - type      : name");
        sLog.outString("playerbots: -------------------------------------------------------------------------------------------------------");

        for (std::map<PerformanceMetric, std::map<std::string, PerformanceData*> >::iterator i = data.begin();
             i != data.end(); ++i)
        {
            std::map<std::string, PerformanceData*> pdMap = i->second;

            const char* key = "?";
            switch (i->first)
            {
                case PERF_MON_TRIGGER: key = "Trigger"; break;
                case PERF_MON_VALUE:   key = "Value";   break;
                case PERF_MON_ACTION:  key = "Action";  break;
                case PERF_MON_RNDBOT:  key = "RndBot";  break;
                case PERF_MON_TOTAL:   key = "Total";   break;
                default: break;
            }

            std::vector<std::string> names;
            for (std::map<std::string, PerformanceData*>::iterator j = pdMap.begin(); j != pdMap.end(); ++j)
                names.push_back(j->first);

            std::sort(names.begin(), names.end(),
                      [&pdMap](std::string const& a, std::string const& b)
                      { return pdMap.at(a)->totalTime < pdMap.at(b)->totalTime; });

            uint64_t typeTotalTime = 0;
            uint64_t typeMinTime = 0xFFFFFFFFu;
            uint64_t typeMaxTime = 0;
            uint32_t typeCount = 0;

            for (std::vector<std::string>::iterator k = names.begin(); k != names.end(); ++k)
            {
                PerformanceData* pd = pdMap[*k];
                typeTotalTime += pd->totalTime;
                typeCount += pd->count;
                if (typeMinTime > pd->minTime)
                    typeMinTime = pd->minTime;
                if (typeMaxTime < pd->maxTime)
                    typeMaxTime = pd->maxTime;

                float perc = (float)pd->totalTime / (float)fullTickTotalTime * 100.0f;
                float time = (float)pd->totalTime / fullTickCount / 1000.0f;
                float minTime = (float)pd->minTime / 1000.0f;
                float maxTime = (float)pd->maxTime / 1000.0f;
                float avg = (float)pd->totalTime / (float)pd->count / 1000.0f;
                float amount = (float)pd->count / fullTickCount;

                std::string disName = *k;
                if (!fullStack && disName.find("|") != std::string::npos)
                    disName = disName.substr(0, disName.find("|")) + "]";

                if (perc >= 0.1f || avg >= 0.25f || pd->maxTime > 1000)
                {
                    sLog.outString("playerbots: %7.3f%% %9.3fms | %7.1f .. %7.1f (%10.3f of %10.2f) - %-6s    : %s",
                                   perc, time, minTime, maxTime, avg, amount, key, disName.c_str());
                }
            }

            if (i->first != PERF_MON_TOTAL && typeCount > 0)
            {
                float tPerc = (float)typeTotalTime / (float)fullTickTotalTime * 100.0f;
                float tTime = (float)typeTotalTime / fullTickCount / 1000.0f;
                float tMinTime = (float)typeMinTime / 1000.0f;
                float tMaxTime = (float)typeMaxTime / 1000.0f;
                float tAvg = (float)typeTotalTime / (float)typeCount / 1000.0f;
                float tAmount = (float)typeCount / fullTickCount;
                sLog.outString("playerbots: %7.3f%% %9.3fms | %7.1f .. %7.1f (%10.3f of %10.2f) - %-6s    : %s",
                               tPerc, tTime, tMinTime, tMaxTime, tAvg, tAmount, key, "Total");
            }
            sLog.outString("playerbots:  ");
        }
    }
}

void PerfMonitor::Reset()
{
    for (std::map<PerformanceMetric, std::map<std::string, PerformanceData*> >::iterator i = data.begin();
         i != data.end(); ++i)
    {
        std::map<std::string, PerformanceData*> pdMap = i->second;
        for (std::map<std::string, PerformanceData*>::iterator j = pdMap.begin(); j != pdMap.end(); ++j)
        {
            PerformanceData* pd = j->second;
            std::lock_guard<std::mutex> guard(pd->lock);
            pd->minTime = 0;
            pd->maxTime = 0;
            pd->totalTime = 0;
            pd->count = 0;
        }
    }
}
