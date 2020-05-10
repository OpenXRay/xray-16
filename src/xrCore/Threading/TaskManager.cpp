#include "stdafx.h"

#include "ScopeLock.hpp"

#include "Task.hpp"
#include "TaskManager.hpp"

#ifdef USE_TBB_PARALLEL
#include <thread>
#include <tbb/task_scheduler_init.h>
#endif

xr_unique_ptr<TaskManagerBase> TaskScheduler;

#ifdef USE_TBB_PARALLEL
TaskManagerBase::TaskManagerBase() : taskerSleepTime(2), shouldStop(true) {}
#else
TaskManagerBase::TaskManagerBase() {}
#endif // USE_TBB_PARALLEL

void TaskManagerBase::Initialize()
{
#ifdef USE_TBB_PARALLEL
    if (!shouldStop)
        return;

    shouldStop = false;
    Threading::SpawnThread(taskManagerThread, "X-Ray Task Scheduler thread", 0, this);
#endif
}

void TaskManagerBase::Destroy()
{
#ifdef USE_TBB_PARALLEL
    if (shouldStop)
        return;

    shouldStop = true;
    mainThreadExit.Wait();
#endif
}

bool TaskManagerBase::TaskQueueIsEmpty() const
{
#ifdef USE_TBB_PARALLEL
    return tasks.empty();
#else
    return true;
#endif // USE_TBB_PARALLEL
}

#ifdef USE_TBB_PARALLEL
void TaskManagerBase::taskManagerThread(void* thisPtr)
{
    tbb::task_scheduler_init init;

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
#endif // USE_TBB_PARALLEL

void TaskManagerBase::AddTask(pcstr name, Task::TaskFunc taskFunc,
    Task::IsAllowedCallback callback /*= nullptr*/, Task::DoneCallback done /*= nullptr*/,
    Event* doneEvent /*= nullptr*/)
{
#ifdef USE_TBB_PARALLEL
    Task* task = new (tbb::task::allocate_root()) Task(name, std::move(taskFunc),
        std::move(callback), std::move(done), doneEvent);

    if (!task->isExecutionAllowed)
    {
        SpawnTask(task, true);
        return;
    }

    lock.Enter();
    tasks.emplace_back(task);
    lock.Leave();
#else
    Task task(name, std::move(taskFunc), std::move(callback), std::move(done), doneEvent);
    SpawnTask(&task); // SpawnTask can be overriden, so let's call it and execute there
#endif // USE_TBB_PARALLEL
}

void TaskManagerBase::RemoveTask(Task::TaskFunc&& func)
{
#ifdef USE_TBB_PARALLEL
    ScopeLock scope(&lock);

    xr_vector<Task*>::iterator it;
    const auto search = [&]()
    {
        it = std::find_if(tasks.begin(), tasks.end(), [&](Task* task)
        {
            return func == task->task;
        });
        return it;
    };
    if (search() != tasks.end())
    {
        Task::destroy(**it);
        tasks.erase(it);
    }
#endif // USE_TBB_PARALLEL
}

void TaskManagerBase::RemoveTasksWithName(pcstr name)
{
#ifdef USE_TBB_PARALLEL
    ScopeLock scope(&lock);

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
        Task::destroy(**it);
        tasks.erase(it);
    }
#endif // USE_TBB_PARALLEL
}

void TaskManagerBase::SpawnTask(Task* task, bool shortcut /*= false*/)
{
#ifdef USE_TBB_PARALLEL
    if (!shortcut)
    {
        const auto it = std::find(tasks.begin(), tasks.end(), task);
        R_ASSERT3(it != tasks.end(), "Task is deleted from the task manager", task->GetName());

        // Remove it from the queue
        tasks.erase(it);
    }

    // Run it
    tbb::task::spawn(*task);
#else
    UNUSED(shortcut);
    task->execute();
#endif // USE_TBB_PARALLEL
}

void TaskManagerBase::TaskDone(Task* task, u64 executionTime)
{
    if (executionTime > ABNORMAL_EXECUTION_TIME)
    {
        Msg("! Task done after abnormal execution time [%dms] in [%s]", time, task->GetName());
    }
    else if (executionTime > BIG_EXECUTION_TIME)
    {
        Msg("~ Task done after big execution time [%dms] in [%s]", time, task->GetName());
    }
}
