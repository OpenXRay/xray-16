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
#include <SDL_events.h>

#include "Math/fast_lc16.hpp"

#include <random>

#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_E2K)
#include <immintrin.h>
#elif defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64)
#include "sse2neon/sse2neon.h"
#elif defined(XR_ARCHITECTURE_PPC64)
#include <xmmintrin.h>
#else
#error Add your platform here
#endif

xr_unique_ptr<TaskManager> TaskScheduler;

static constexpr size_t OTHER_THREADS_COUNT = 2; // Primary and Secondary thread

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

class TaskAllocator
{
    size_t m_allocated{};
    Task   m_storage[TASK_STORAGE_SIZE];

public:
    Task* allocate()
    {
        Task* task = &m_storage[m_allocated++ & TASK_STORAGE_MASK];
        VERIFY(task->IsFinished());
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
        const auto task_pos = m_tail_pos.fetch_add(1, std::memory_order_acq_rel);
        VERIFY2(task_pos - m_head_pos.load(std::memory_order_acquire) < TASK_STORAGE_SIZE, "Task queue overflow");
        m_storage[task_pos & TASK_STORAGE_MASK] = task;
    }

    Task* pop()
    {
        size_t head_pos = m_head_pos.load(std::memory_order_acquire);
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

struct TaskWorkerStats
{
    size_t allocatedTasks{};
    size_t pushedTasks{};
    size_t finishedTasks{};
};

class TaskWorker : public TaskQueue, public TaskWorkerStats
{
public:
    Event            event;
    fast_lc16        random{ this };
    size_t           id    { size_t(-1) };
    std::atomic_bool sleeps{};

    void WakeUpIfNeeded()
    {
        if (!empty() && sleeps.load(std::memory_order_relaxed))
            event.Set(); // Wake up, we have work to do!
    }
} static thread_local s_tl_worker;

class ThreadPriorityHelper
{
    Threading::priority_class m_priority;

public:
    ThreadPriorityHelper()
        : m_priority(Threading::GetCurrentProcessPriorityClass())
    {
        Threading::SetCurrentProcessPriorityClass(Threading::priority_class::realtime);
    }

    ~ThreadPriorityHelper()
    {
        Threading::SetCurrentProcessPriorityClass(m_priority);
    }
};

// Get fast spin-loop timings
void CalcIterations()
{
    [[maybe_unused]] ThreadPriorityHelper priority;

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
    ZoneScoped;
    workers.reserve(std::thread::hardware_concurrency());
    RegisterThisThreadAsWorker();
}

void TaskManager::SpawnThreads()
{
    ZoneScoped;
    CalcIterations();

    const u32 threads = workers.capacity() - OTHER_THREADS_COUNT;
    workerThreads.reserve(threads);

    for (u32 i = 0; i < threads; ++i)
    {
        workerThreads.emplace_back(Threading::RunThread("Task Worker", &TaskManager::TaskWorkerStart, this));
    }
}

TaskManager::~TaskManager()
{
    ZoneScoped;
    shouldStop.store(true, std::memory_order_release);

    // Finish all pending tasks
    while (Task* t = s_tl_worker.pop())
    {
        ExecuteTask(*t);
    }

    UnregisterThisThreadAsWorker();
    {
        std::lock_guard guard{ workersLock };
        for (TaskWorker* worker : workers)
            worker->event.Set();
    }
    for (auto& thread : workerThreads)
    {
        if (thread.joinable())
            thread.join();
    }
}

void TaskManager::RegisterThisThreadAsWorker()
{
    ZoneScoped;
    R_ASSERT2(workers.size() < std::thread::hardware_concurrency(),
        "You must change OTHER_THREADS_COUNT if you want to register more custom threads.");

    std::lock_guard guard{ workersLock };
    s_tl_worker.id = workers.size();
    workers.emplace_back(&s_tl_worker);
}

void TaskManager::UnregisterThisThreadAsWorker()
{
    ZoneScoped;
    std::lock_guard guard{ workersLock };

    shouldPause.store(true, std::memory_order_release);
    while (activeWorkersCount.load(std::memory_order_relaxed))
        Sleep(2);

    s_tl_worker.id = size_t(-1);

    const auto it = std::find(workers.begin(), workers.end(), &s_tl_worker);
    if (it != workers.end())
        workers.erase(it);

    shouldPause.store(false, std::memory_order_release);
}

void TaskManager::SetThreadStatus(bool active)
{
    s_tl_worker.sleeps.store(!active, std::memory_order_relaxed);
    if (active)
        activeWorkersCount.fetch_add(1, std::memory_order_relaxed);
    else
        activeWorkersCount.fetch_sub(1, std::memory_order_relaxed);
}

void TaskManager::TaskWorkerStart()
{
    RegisterThisThreadAsWorker();
    SetThreadStatus(true);

    const u32 fastIterations = ttapi_dwFastIter;

    u32 iteration = 0;
    while (true)
    {
    get_task:
    {
        if (shouldStop.load(std::memory_order_consume))
            break;
        if (shouldPause.load(std::memory_order_consume))
            goto wait;

        Task* task = s_tl_worker.pop();
        if (!task)
            task = TryToSteal();

        if (task)
        {
            ExecuteTask(*task);
            iteration = 0;
            goto get_task;
        }
    }
    //count_spins:
    {
        ++iteration;
        if (iteration < fastIterations)
        {
            _mm_pause();
            goto get_task;
        }
    }
    wait:
    {
        SetThreadStatus(false);
        do
        {
            s_tl_worker.event.Wait(1);
        } while (shouldPause.load(std::memory_order_consume));
        SetThreadStatus(true);

        iteration = 0;
        goto get_task;
    }
    } // while (true)

    SetThreadStatus(false);

    // Finish all pending tasks
    while (Task* t = s_tl_worker.pop())
    {
        ExecuteTask(*t);
    }

    // Prepare to exit
    UnregisterThisThreadAsWorker();
}

Task* TaskManager::TryToSteal() const
{
    std::uniform_int_distribution<u16> dist{ 0, static_cast<u16>(workers.size() - 1) };

    int steal_attempts = 5;
    while (steal_attempts > 0)
    {
        const auto idx = dist(s_tl_worker.random);
        TaskWorker* other = workers[idx];
        if (other == &s_tl_worker)
            continue;
        if (auto* task = other->steal())
        {
            other->WakeUpIfNeeded();
            return task;
        }
        --steal_attempts;
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
    ++s_tl_worker.pushedTasks;
}

void TaskManager::RunTask(Task& task)
{
    ExecuteTask(task);
}

void TaskManager::Wait(const Task& task) const
{
    ZoneScoped;
    while (!task.IsFinished())
    {
        ExecuteOneTask();
        if (s_tl_worker.id == 0 && xrDebug::ProcessingFailure())
            SDL_PumpEvents(); // Necessary to prevent dead locks
    }
}

void TaskManager::WaitForChildren(const Task& task) const
{
    ZoneScoped;
    while (!task.HasChildren())
    {
        ExecuteOneTask();
        if (s_tl_worker.id == 0 && xrDebug::ProcessingFailure())
            SDL_PumpEvents(); // Necessary to prevent dead locks
    }
}

bool TaskManager::ExecuteOneTask() const
{
    Task* task = s_tl_worker.pop();
    if (!task)
        task = TryToSteal();

    if (task)
    {
        ExecuteTask(*task);
        return true;
    }
    return false;
}

Task& TaskManager::CreateTask(const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    return *new (AllocateTask()) Task(taskFunc, data, dataSize);
}

Task& TaskManager::CreateTask(const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    return *new (AllocateTask()) Task(taskFunc, onFinishCallback, data, dataSize);
}

Task& TaskManager::CreateTask(Task& parent, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    IncrementTaskJobsCounter(parent);
    return *new (AllocateTask()) Task(taskFunc, data, dataSize, &parent);
}

Task& TaskManager::CreateTask(Task& parent, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    IncrementTaskJobsCounter(parent);
    return *new (AllocateTask()) Task(taskFunc, onFinishCallback, data, dataSize, &parent);
}

Task& TaskManager::AddTask(const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    auto& task = CreateTask(taskFunc, dataSize, data);
    PushTask(task);
    return task;
}

Task& TaskManager::AddTask(const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    auto& task = CreateTask(onFinishCallback, taskFunc, dataSize, data);
    PushTask(task);
    return task;
}

Task& TaskManager::AddTask(Task& parent, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    auto& task = CreateTask(parent, taskFunc, dataSize, data);
    PushTask(task);
    return task;
}

Task& TaskManager::AddTask(Task& parent, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize /*= 0*/, void* data /*= nullptr*/)
{
    auto& task = CreateTask(parent, onFinishCallback, taskFunc, dataSize, data);
    PushTask(task);
    return task;
}

size_t TaskManager::GetWorkersCount() const
{
    return workers.size();
}

size_t TaskManager::GetCurrentWorkerID()
{
    return s_tl_worker.id;
}

void TaskManager::GetStats(size_t& allocated, size_t& pushed, size_t& finished)
{
    std::lock_guard guard{ workersLock };
    for (const TaskWorker* worker : workers)
    {
        allocated += worker->allocatedTasks;
        pushed += worker->pushedTasks;
        finished += worker->finishedTasks;
    }
}
