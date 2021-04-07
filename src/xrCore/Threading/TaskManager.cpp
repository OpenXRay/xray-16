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

#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_E2K)
#include <immintrin.h>
#elif defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64)
#include "sse2neon/sse2neon.h"
#else
#error Add your platform here
#endif

xr_unique_ptr<TaskManager> TaskScheduler;

static constexpr size_t OTHER_THREADS_COUNT = 1; // Primary thread

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
    template <size_t Power>
    static constexpr size_t Get()
    {
        return PowerOfTwo<Power>::value;
    }
};

static constexpr size_t TASK_STORAGE_SIZE = TaskStorageSize::Get<12>(); // 4096 tasks
static constexpr size_t TASK_STORAGE_MASK = TASK_STORAGE_SIZE - 1;

class FallbackTaskAllocator
{
    std::atomic_size_t m_allocated{};
    Task               m_storage[TASK_STORAGE_SIZE];

public:
    Task* allocate()
    {
        const auto task_pos = m_allocated.fetch_add(1, std::memory_order_acq_rel);
        Task* task = &m_storage[task_pos & TASK_STORAGE_MASK];
        R_ASSERT2(task->IsFinished(), "Both thread local and fallback task allocator are full. "
            "Too much workload for one thread.");
        return task;
    }

    size_t get_allocated_count() const
    {
        return m_allocated.load(std::memory_order_relaxed);
    }
} static s_task_allocator_mt;

class TaskAllocator
{
    size_t m_allocated{};
    Task   m_storage[TASK_STORAGE_SIZE];

public:
    Task* allocate()
    {
        Task* task = &m_storage[m_allocated++ & TASK_STORAGE_MASK];
        if (!task->IsFinished()) // XXX: mark as unlikely
        {
            --m_allocated;
            return s_task_allocator_mt.allocate();
        }
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

    Task* steal()
    {
        return pop();
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

class TaskWorkerStats
{
public:
    size_t allocatedTasks{};
    size_t pushedTasks{};
    size_t finishedTasks{};
};

class TaskWorker : public TaskQueue, public TaskWorkerStats
{
public:
    std::atomic<TaskWorker*> steal_from{};
    std::atomic_bool sleeps{};
    Event event;
} static thread_local s_tl_worker;

static TaskWorker* s_main_thread_worker = nullptr;

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

TaskManager::TaskManager()
{
    s_main_thread_worker = &s_tl_worker;

    const u32 threads = CPU::ID.n_threads - OTHER_THREADS_COUNT;
    workers.reserve(threads);
    for (u32 i = 0; i < threads; ++i)
    {
        Threading::SpawnThread(task_worker_entry, "Task Worker", 0, this);
    }
    CalcIterations();
    while (threads != workersCount.load(std::memory_order_consume))
    {
        Sleep(2);
    }
    for (TaskWorker* worker : workers)
        worker->event.Set();
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
        Sleep(2);
    }
    for (TaskWorker* worker : workers)
        worker->event.Set();

    s_main_thread_worker = nullptr;
}

void TaskManager::task_worker_entry(void* this_ptr)
{
    TaskManager& self = *static_cast<TaskManager*>(this_ptr);
    self.TaskWorkerStart();
}

void TaskManager::SetThreadStatus(bool active)
{
    s_tl_worker.sleeps.store(!active, std::memory_order_relaxed);
    if (active)
        activeWorkersCount.fetch_add(1, std::memory_order_relaxed);
    else
        activeWorkersCount.fetch_sub(1, std::memory_order_relaxed);
}

void TaskManager::WakeUpIfNeeded()
{
    const auto overall = workersCount.load(std::memory_order_relaxed);
    const auto active = activeWorkersCount.load(std::memory_order_relaxed);
    if (active < overall)
    {
        auto* steal_from = &s_tl_worker;
        for (auto* worker : workers)
        {
            if (worker == &s_tl_worker)
                continue;
            if (worker->sleeps.load(std::memory_order_relaxed))
            {
                worker->steal_from.store(steal_from, std::memory_order_relaxed);
                worker->event.Set();
                break;
            }
        }
    }
}

void TaskManager::TaskWorkerStart()
{
    {
        ScopeLock scope(&workersLock);
        workers.emplace_back(&s_tl_worker);
    }
    workersCount.fetch_add(1, std::memory_order_release);
    activeWorkersCount.fetch_add(1, std::memory_order_relaxed);

    s_tl_worker.event.Wait();

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
        task = s_tl_worker.pop();
        if (task)
            goto execute;
    }
    check_main_queue:
    {
        task = s_main_thread_worker->steal();
        if (task)
            goto execute;
    }
    steal:
    {
        task = TryToSteal(&s_tl_worker);
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
        iteration = 0;
        SetThreadStatus(false);
        s_tl_worker.event.Wait();
        SetThreadStatus(true);
        auto* stealFrom = s_tl_worker.steal_from.load(std::memory_order_relaxed);
        if (stealFrom)
        {
            while (Task* t = stealFrom->steal())
            {
                ExecuteTask(*t);
            }
            s_tl_worker.steal_from.store(nullptr, std::memory_order_relaxed);
        }
        goto check_own_queue;
    }
    } // while (true)

    // Finish all pending tasks
    while (Task* t = s_tl_worker.pop())
    {
        ExecuteTask(*t);
    }

    // Prepare to exit
    workersCount.fetch_sub(1, std::memory_order_release);
    s_tl_worker.event.Wait(); // prevent crash when other thread tries to steal
}

Task* TaskManager::TryToSteal(TaskWorker* thief)
{
    const auto count = workersCount.load(std::memory_order_relaxed);
    if (count == 1)
    {
        if (&s_tl_worker == s_main_thread_worker)
            return workers[0]->steal();
        return nullptr; // thread itself
    }

    TaskWorker* other = workers[random.randI(count)];
    if (other != thief)
    {
        auto* task = other->steal();
        if (!other->empty() && other->sleeps.load(std::memory_order_relaxed))
            other->event.Set(); // Wake up, you have work to do!
        return task;
    }
    return nullptr;
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
        VERIFY2(unfinishedJobs >= 0, "The same task was executed two times.");
        it->Finish();
        if (unfinishedJobs || !it->m_data.parent)
            break;
    }
    ++s_tl_worker.finishedTasks;
}

Task* TaskManager::AllocateTask()
{
    ++s_tl_worker.allocatedTasks;
    return s_tl_allocator.allocate();
}

void TaskManager::IncrementTaskJobsCounter(Task& parent)
{
    VERIFY2(parent.m_data.jobs.load(std::memory_order_relaxed) > 0, "Adding child task to a parent that has already finished.");
    [[maybe_unused]] const auto prev = parent.m_data.jobs.fetch_add(1, std::memory_order_relaxed);
    VERIFY2(prev != std::numeric_limits<decltype(prev)>::max(), "Max jobs overflow. (too much children)");
}

void TaskManager::PushTask(Task& task)
{
    s_tl_worker.push(&task);
    WakeUpIfNeeded();
    ++s_tl_worker.pushedTasks;
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
    WakeUpIfNeeded();

    Task* task = s_tl_worker.pop();
    if (!task)
        task = TryToSteal(&s_tl_worker);

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

size_t TaskManager::GetWorkersCount() const
{
    return workersCount.load(std::memory_order_relaxed) + OTHER_THREADS_COUNT;
}

size_t TaskManager::GetActiveWorkersCount() const
{
    return activeWorkersCount.load(std::memory_order_relaxed) + OTHER_THREADS_COUNT;
}

void TaskManager::GetStats(size_t& allocated, size_t& allocatedWithFallback, size_t& pushed, size_t& finished)
{
    allocatedWithFallback += s_task_allocator_mt.get_allocated_count();

    allocated += s_main_thread_worker->allocatedTasks;
    pushed += s_main_thread_worker->pushedTasks;
    finished += s_main_thread_worker->finishedTasks;

    ScopeLock scope(&workersLock);
    for (TaskWorker* worker : workers)
    {
        allocated += worker->allocatedTasks;
        pushed += worker->pushedTasks;
        finished += worker->finishedTasks;
    }
}
