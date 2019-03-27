#include "stdafx.h"

#include "ScopeLock.hpp"

#include "Task.hpp"
#include "TaskManager.hpp"

#include <thread>

xr_unique_ptr<TaskManagerBase> TaskScheduler;

TaskManagerBase::TaskManagerBase() : taskerSleepTime(2), shouldStop(true) {}

void TaskManagerBase::Initialize()
{
    if (!shouldStop)
        return;

    shouldStop = false;
    thread_spawn(taskManagerThread, "X-Ray Task Scheduler thread", 0, this);
    thread_spawn(taskWatcherThread, "X-Ray Task Watcher thread", 0, this);
}

void TaskManagerBase::Destroy()
{
    if (shouldStop)
        return;

    shouldStop = true;
    mainThreadExit.Wait();
    watcherThreadExit.Wait();
}

bool TaskManagerBase::TaskQueueIsEmpty() const
{
    return tasks.empty() && tasksInExecution.empty();
}

void TaskManagerBase::taskManagerThread(void* thisPtr)
{
    TaskManagerBase& self = *static_cast<TaskManagerBase*>(thisPtr);

    while (!self.shouldStop)
    {
        self.lock.Enter();
        if (self.tasks.empty())
        {
            self.lock.Leave();
            Sleep(self.taskerSleepTime);
            continue;
        }

        for (Task* task : self.tasks)
        {
            if (task->CheckIfExecutionAllowed())
            {
                self.SpawnTask(task);
                break; // Iterators are invalid now, we should break here
            }
        }
        self.lock.Leave();
    }
    self.mainThreadExit.Set();
}

void TaskManagerBase::taskWatcherThread(void* thisPtr)
{
    TaskManagerBase& self = *static_cast<TaskManagerBase*>(thisPtr);
    bool calmDown = false;

    while (!self.shouldStop)
    {
        self.executionLock.Enter();
        if (self.tasksInExecution.empty())
        {
            self.executionLock.Leave();
            Sleep(WATCHER_CALM_DOWN_PERIOD);
            continue;
        }

        for (Task* task : self.tasksInExecution)
        {
            if (!task->IsStarted())
                continue;

            const u64 time = task->GetElapsedMs();

            if (time > ABNORMAL_EXECUTION_TIME)
            {
                calmDown = true;
                Msg("! Abnormal task execution time [%dms] in [ %s ]", time, task->GetName());
            }
            
#ifdef PROFILE_TASKS
            bool tooLong;
            bool veryLong;

            else if (task->IsComplex())
            {
                tooLong = time > COMPLEX_TASK_ACCEPTABLE_EXECUTION_TIME;
                veryLong = time > COMPLEX_TASK_ACCEPTABLE_EXECUTION_TIME * 2;
            }
            else
            {
                tooLong = time > SIMPLE_TASK_ACCEPTABLE_EXECUTION_TIME;
                veryLong = time > SIMPLE_TASK_ACCEPTABLE_EXECUTION_TIME * 2;
            }

            if (veryLong)
            {
                calmDown = true;
                Msg("! Task [%s] runs too long %dms", task->GetName(), time);
            }
            else if (tooLong)
            {
                calmDown = true;
                Msg("~ Task [%s] runs very long %dms", task->GetName(), time);
            }
#endif
        }
        self.executionLock.Leave();
        if (calmDown)
        {
            Sleep(WATCHER_CALM_DOWN_PERIOD);
            calmDown = false;
        }
    }
    self.watcherThreadExit.Set();
}

void TaskManagerBase::AddTask(pcstr name, Task::Type type, Task::TaskFunc taskFunc,
    Task::IsAllowedCallback callback, Task::DoneCallback done /*= nullptr*/)
{
    Task* task = new (tbb::task::allocate_root()) Task(name, type, taskFunc, callback, done);

    lock.Enter();
    tasks.push_back(task);
    lock.Leave();
}

void TaskManagerBase::RemoveTasksWithName(pcstr name)
{
    ScopeLock scope(&lock);

    for (Task* task : tasks)
    {
        if (0 == xr_strcmp(name, task->GetName()))
            Task::destroy(*task);
    }

    xr_vector<Task*>::iterator it;

    const auto search = [&]()
    {
        it = std::find_if(tasks.begin(), tasks.end(), [&](Task* task)
        {
            return 0 == xr_strcmp(name, task->GetName());
        });
        return it;
    };

    while (search() != tasks.end())
    {
        tasks.erase(it);
    }
}

void TaskManagerBase::RemoveTasksWithType(Task::Type type)
{
    ScopeLock scope(&lock);

    for (Task* task : tasks)
    {
        if (task->GetType() == type)
            Task::destroy(*task);
    }

    xr_vector<Task*>::iterator it;

    const auto search = [&]()
    {
        it = std::find_if(tasks.begin(), tasks.end(), [&](Task* task)
        {
            return type == task->GetType();
        });
        return it;
    };

    while (search() != tasks.end())
    {
        tasks.erase(it);
    }
}

void TaskManagerBase::SpawnTask(Task* task)
{
    const auto it = std::find(tasks.begin(), tasks.end(), task);
    R_ASSERT3(it != tasks.end(), "Task is deleted from the task manager", task->GetName());

    // Remove it from the queue
    tasks.erase(it);
    
    // Watch it
    executionLock.Enter();
    tasksInExecution.push_back(task);
    executionLock.Leave();

    // Run it
    tbb::task::spawn(*task);
}

void TaskManagerBase::TaskDone(Task* task, u64 executionTime)
{
    ScopeLock scope(&executionLock);

    const auto it = std::find(tasksInExecution.begin(), tasksInExecution.end(), task);
    R_ASSERT3(it != tasksInExecution.end(), "Task is deleted from the task watcher", task->GetName());

    tasksInExecution.erase(it);
}
