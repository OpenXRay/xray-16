#include "stdafx.h"

#include "xrCore/Threading/ScopeLock.hpp"
#include "IGameFont.hpp"
#include "IPerformanceAlert.hpp"

#include "TaskScheduler.hpp"

#include <cinttypes>

void TaskManager::TaskDone(Task* task, u64 executionTime)
{
    TaskManagerBase::TaskDone(task, executionTime);

    if (psDeviceFlags.test(rsStatistic))
    {
        ScopeLock scope(&statisticsLock);
        statistics.push_back(static_cast<float>(executionTime));
    }
}

void TaskManager::DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert)
{
    ScopeLock scope(&statisticsLock);

    const size_t tasksDone = statistics.size();
    float averageExecutionTime = 0.f;
    float minExecutionTime = 0.f;
    float maxExecutionTime = 0.f;

    for (float ms : statistics)
    {
        minExecutionTime = std::min(minExecutionTime, ms);
        maxExecutionTime = std::max(maxExecutionTime, ms);
        averageExecutionTime += ms;
    }

    if (tasksDone > 0)
        averageExecutionTime /= static_cast<float>(tasksDone);

    font.OutNext("Task scheduler:");
    font.OutNext("- tasks:       %zu", tasksDone);
    font.OutNext("- average:     %2.2f ms", averageExecutionTime);
    font.OutNext("- min:         %2.2f ms", minExecutionTime);
    font.OutNext("- max:         %2.2f ms", maxExecutionTime);

    if (alert && averageExecutionTime > BIG_EXECUTION_TIME)
        alert->Print(font, "Tasks    > %dms:  %3.1f", BIG_EXECUTION_TIME, averageExecutionTime);

    statistics.clear();
}
