#pragma once

#include "xrCore/Threading/TaskManager.hpp"

class IGameFont;
class IPerformanceAlert;

class TaskManager : public TaskManagerBase
{
    Lock statisticsLock;
    xr_vector<float> statistics;

    void TaskDone(Task* task, u64 executionTime) override;

public:
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;
};
