#pragma once

#include "Event.hpp"
#include "Task.hpp"

//#define TASKS_PROFILER

#ifdef PROFILE_TASKS
#ifdef DEBUG // Because it's slower
constexpr u64 SIMPLE_TASK_ACCEPTABLE_EXECUTION_TIME = 16; // ms
constexpr u64 COMPLEX_TASK_ACCEPTABLE_EXECUTION_TIME = 32; // ms
#else
constexpr u64 SIMPLE_TASK_ACCEPTABLE_EXECUTION_TIME = 8; // ms
constexpr u64 COMPLEX_TASK_ACCEPTABLE_EXECUTION_TIME = 16; // ms
#endif
#endif

constexpr u64 ABNORMAL_EXECUTION_TIME = 1000; // ms
constexpr u64 BIG_EXECUTION_TIME = 500; // ms

constexpr u64 WATCHER_CALM_DOWN_PERIOD = 5; // ms

class XRCORE_API TaskManagerBase
{
    xr_vector<Task*> tasks;
    xr_vector<Task*> tasksInExecution;

    Lock lock;
    Lock executionLock;
    Event mainThreadExit;
    Event watcherThreadExit;
    u32 taskerSleepTime;
    bool shouldStop;

    static void taskManagerThread(void* thisPtr);
    static void taskWatcherThread(void* thisPtr);

    void SpawnTask(Task* task);

protected:
    friend class Task;
    virtual void TaskDone(Task* task, u64 executionTime);

public:
    TaskManagerBase();

    void Initialize();
    void Destroy();

    bool TaskQueueIsEmpty() const;

    void AddTask(pcstr name, Task::Type type, Task::TaskFunc taskFunc,
        Task::IsAllowedCallback callback, Task::DoneCallback done = nullptr);

    void RemoveTasksWithName(pcstr name);
    void RemoveTasksWithType(Task::Type type);

    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) = 0;
};

extern XRCORE_API xr_unique_ptr<TaskManagerBase> TaskScheduler;
