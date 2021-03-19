/*
    Copyright (c) 2014-2021 OpenXRay

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#include "stdafx.h"

#include "Task.hpp"
#include "TaskManager.hpp"

#include "ScopeLock.hpp"

#include <thread>
#include <mutex>

#include <immintrin.h>

xr_unique_ptr<TaskManager> TaskScheduler;

static constexpr size_t OTHER_THREADS_COUNT = 1;

static u32 ttapi_dwFastIter = 0;

class TaskStorageSize
{
    template <int E, typename Dummy = void> // XXX: remove Dummy workaround when gcc will finally match the standard
    struct PowerOfTwo
    {
        static constexpr size_t value = 2 * PowerOfTwo<E - 1>::value;
    };

    template <typename Dummy>
    struct PowerOfTwo<0, Dummy>
    {
        static constexpr size_t value = 1;
    };
    
public:
    static constexpr size_t Get()
    {
        return PowerOfTwo<10>::value; // 1024 tasks
    }
};

static constexpr size_t TASK_STORAGE_SIZE = TaskStorageSize::Get();
static constexpr size_t TASK_STORAGE_MASK = TASK_STORAGE_SIZE - 1;

class TaskAllocator
{
    size_t m_allocated{};
    Task   m_storage[TASK_STORAGE_SIZE];

public:
    Task* allocate()
    {
        Task* task = &m_storage[m_allocated++ & TASK_STORAGE_MASK];
        VERIFY2(task->IsFinished(), "Not enough memory in task allocator");
        return task;
    }
} static thread_local s_tl_allocator;

// Multi producer
// Multi consumer
class TaskQueue
{
    std::atomic_size_t  m_head_pos{};
    std::atomic_size_t  m_tail_pos{};
    Task*               m_storage[TASK_STORAGE_SIZE]{};

public:
    void push(Task* task)
    {
        const auto task_pos = m_tail_pos.fetch_add(1, std::memory_order_relaxed);
        VERIFY2(task_pos - m_head_pos.load(std::memory_order_relaxed) < TASK_STORAGE_SIZE, "Task queue overflow");
        m_storage[task_pos & TASK_STORAGE_MASK] = task;
    }

    Task* pop()
    {
        size_t head_pos = m_head_pos.load(std::memory_order_relaxed);
        Task* task = m_storage[head_pos & TASK_STORAGE_MASK];
        if (!task)
            return nullptr;

        const bool success = m_head_pos.compare_exchange_weak(head_pos, head_pos + 1, std::memory_order_acq_rel);
        if (success)
        {
            m_storage[head_pos & TASK_STORAGE_MASK] = nullptr;
            return task;
        }
        return nullptr;
    }

    size_t size() const
    {
        return m_head_pos - m_tail_pos;
    }

    bool empty() const
    {
        return size() == 0;
    }
};

class TaskWorker : public TaskQueue
{
public:
    Event event;
};

class ThreadPriorityHelper
{
#ifdef XR_PLATFORM_WINDOWS
    DWORD m_priority;

public:
    ThreadPriorityHelper()
        : m_priority(GetPriorityClass(GetCurrentProcess()))
    {
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    }

    ~ThreadPriorityHelper()
    {
        SetPriorityClass(GetCurrentProcess(), m_priority);
    }
#else
    // XXX: add other platforms
public:
    ThreadPriorityHelper() = default;
#endif
};

// Get fast spin-loop timings
void CalcIterations()
{
    ThreadPriorityHelper priority;

    volatile bool dummy = false;
    const u64 frequency = CPU::qpc_freq;
    const u32 iterations = 100000000; // approximately 1 second
    const u64 start = CPU::QPC();
    for (u32 i = 0; i < iterations; ++i)
    {
        if (dummy)
            break;
        _mm_pause();
    }
    const u64 end = CPU::QPC();
    // We want 1/50000 (0.02ms) fast spin-loop
    ttapi_dwFastIter = u32((iterations * frequency) / ((end - start) * 50000));
}

TaskManager::TaskManager() : shouldStop(true)
{
    const u32 threads = std::thread::hardware_concurrency() - OTHER_THREADS_COUNT;
    for (u32 i = 0; i < threads; ++i)
    {
        Threading::SpawnThread(task_worker_entry, "X-Ray Task Worker Thread", 0, this);
    }
    CalcIterations();
    while (threads != workersCount.load(std::memory_order_consume))
    {
        Sleep(1);
    }
    shouldStop.store(false, std::memory_order_release);
}

TaskManager::~TaskManager()
{
    shouldStop.store(true, std::memory_order_release);
    {
        ScopeLock scope(&workersLock);
        for (TaskWorker* worker : workers)
            worker->event.Set();
    }
    while (workersCount.load(std::memory_order_consume))
    {
        Sleep(1);
    }
}

void TaskManager::task_worker_entry(void* this_ptr)
{
    TaskManager& self = *static_cast<TaskManager*>(this_ptr);
    self.TaskWorkerStart();
}

void TaskManager::TaskWorkerStart()
{
    TaskWorker worker;
    {
        ScopeLock scope(&workersLock);
        workers.emplace_back(&worker);
    }
    workersCount.fetch_add(1, std::memory_order_release);

    while (shouldStop.load(std::memory_order_consume))
        Sleep(1);

    const u32 fastIterations = ttapi_dwFastIter;

    int iteration = 0;
    Task* task;
    while (true)
    {
        goto check_own_queue;

    execute:
    {
        ExecuteTask(*task);
        iteration = 0;
    }
    check_own_queue:
    {
        task = worker.pop();
        if (task)
            goto execute;
    }
    steal:
    {
        task = GetWorkerToStealFrom(&worker)->pop();
        if (task)
            goto execute;
    }
    count_spins:
    {
        if (shouldStop.load(std::memory_order_consume))
            break;

        ++iteration;
        if (iteration < fastIterations)
        {
            _mm_pause();
            goto check_own_queue;
        }
    }
    wait:
    {
        worker.event.Wait();
        goto check_own_queue;
    }
    } // while (true)

    // Decrement first to minimize the chance of accessing
    // removed worker in GetWorkerForWorkload and GetWorkerToStealFrom
    workersCount.fetch_sub(1, std::memory_order_release);
    {
        ScopeLock scope(&workersLock);
        const auto it = std::find(workers.begin(), workers.end(), &worker);
        workers.erase(it);
    }
}

void TaskManager::ExecuteTask(Task& task)
{
    task.Execute();
    FinalizeTask(task);
}

void TaskManager::FinalizeTask(Task& task)
{
    for (Task* it = &task; ; it = it->m_data.parent)
    {
        const auto unfinishedJobs = it->m_data.jobs.fetch_sub(1, std::memory_order_acq_rel) - 1; // fetch_sub returns previous value
        it->Finish();
        finishedTasks.fetch_add(1, std::memory_order_relaxed);
        if (unfinishedJobs || !it->m_data.parent)
            break;
    }
}

Task* TaskManager::AllocateTask()
{
    allocatedTasks.fetch_add(1, std::memory_order_relaxed);
    return s_tl_allocator.allocate();
}

void TaskManager::IncrementTaskJobsCounter(Task& parent)
{
    VERIFY2(parent.m_data.jobs.load(std::memory_order_relaxed) > 0, "Adding child task to a parent that has already finished.");
    [[maybe_unused]] const auto prev = parent.m_data.jobs.fetch_add(1, std::memory_order_acq_rel);
    VERIFY2(prev != std::numeric_limits<decltype(prev)>::max(), "Max jobs overflow. (too much children)");
}

void TaskManager::PushTask(Task& task)
{
    const auto worker = GetWorkerForWorkload();
    worker->push(&task);
    worker->event.Set();
    pushedTasks.fetch_add(1, std::memory_order_relaxed);
}

void TaskManager::Wait(const Task& task)
{
    while (!task.IsFinished())
    {
        ExecuteOneTask();
    }
}

void TaskManager::WaitForChildren(const Task& task)
{
    while (!task.HasChildren())
    {
        ExecuteOneTask();
    }
}

bool TaskManager::ExecuteOneTask()
{
    Task* task = GetWorkerToStealFrom(nullptr)->pop();
    if (task)
    {
        ExecuteTask(*task);
        return true;
    }
    return false;
}

Task& TaskManager::CreateTask(pcstr name, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    return *new (AllocateTask()) Task(name, taskFunc, data, dataSize);
}

Task& TaskManager::CreateTask(pcstr name, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    return *new (AllocateTask()) Task(name, taskFunc, onFinishCallback, data, dataSize);
}

Task& TaskManager::CreateTask(Task& parent, pcstr name, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    IncrementTaskJobsCounter(parent);
    return *new (AllocateTask()) Task(name, taskFunc, data, dataSize, &parent);
}

Task& TaskManager::CreateTask(Task& parent, pcstr name, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    IncrementTaskJobsCounter(parent);
    return *new (AllocateTask()) Task(name, taskFunc, onFinishCallback, data, dataSize, &parent);
}

Task& TaskManager::AddTask(pcstr name, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    auto& task = CreateTask(name, taskFunc, dataSize, data);
    PushTask(task);
    return task;
}

Task& TaskManager::AddTask(pcstr name, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    auto& task = CreateTask(name, onFinishCallback, taskFunc, dataSize, data);
    PushTask(task);
    return task;
}

Task& TaskManager::AddTask(Task& parent, pcstr name, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    auto& task = CreateTask(parent, name, taskFunc, dataSize, data);
    PushTask(task);
    return task;
}

Task& TaskManager::AddTask(Task& parent, pcstr name, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    auto& task = CreateTask(parent, name, onFinishCallback, taskFunc, dataSize, data);
    PushTask(task);
    return task;
}

TaskWorker* TaskManager::GetWorkerForWorkload()
{
    const auto idx = currentWorker.fetch_add(1, std::memory_order_relaxed);
    TaskWorker* queue = workers[idx % workersCount.load(std::memory_order_relaxed)];
    return queue;
}

TaskWorker* TaskManager::GetWorkerToStealFrom(TaskWorker* thief)
{
    while (true)
    {
        const auto count = workersCount.load(std::memory_order_relaxed);
        if (count == 1)
            return workers[0]; // safe, but maybe not optimal
        TaskWorker* worker = workers[random.randI(count)];
        if (worker != thief)
            return worker; 
    }
}

size_t TaskManager::GetWorkersCount() const
{
    return workersCount.load(std::memory_order_relaxed) + OTHER_THREADS_COUNT;
}

size_t TaskManager::GetAllocatedCount() const
{
    return allocatedTasks.load(std::memory_order_relaxed);
}

size_t TaskManager::GetPushedCount() const
{
    return pushedTasks.load(std::memory_order_relaxed);
}

size_t TaskManager::GetFinishedCount() const
{
    return finishedTasks.load(std::memory_order_relaxed);
}
