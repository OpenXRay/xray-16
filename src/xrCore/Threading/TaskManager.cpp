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
    CRandom          random{ s32(std::intptr_t(this)) };
    size_t           id    { size_t(-1) };
    std::atomic_bool sleeps{ true };

    void WakeUpIfNeeded()
    {
        if (!empty() && sleeps.load(std::memory_order_relaxed))
            event.Set(); // Wake up, we have work to do!
    }
} static thread_local s_tl_worker;

static TaskWorker* s_main_thread_worker = nullptr;

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
    workers.reserve(std::thread::hardware_concurrency());

    s_main_thread_worker = &s_tl_worker;
    RegisterThisThreadAsWorker();

    const u32 threads = std::thread::hardware_concurrency() - OTHER_THREADS_COUNT;
    for (u32 i = 0; i < threads; ++i)
    {
        Threading::SpawnThread([](void* this_ptr)
        {
            TaskManager& self = *static_cast<TaskManager*>(this_ptr);
            self.TaskWorkerStart();
        }, "Task Worker", 0, this);
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
        std::lock_guard guard{ workersLock };
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

void TaskManager::RegisterThisThreadAsWorker()
{
    std::lock_guard guard{ workersLock };
    s_tl_worker.id = workers.size();
    workers.emplace_back(&s_tl_worker);
}

void TaskManager::UnregisterThisThreadAsWorker()
{
    std::lock_guard guard{ workersLock };
    s_tl_worker.id = size_t(-1);
    workers.emplace_back(&s_tl_worker);
}

void TaskManager::TaskWorkerStart()
{
    RegisterThisThreadAsWorker();
    workersCount.fetch_add(1, std::memory_order_release);
    s_tl_worker.event.Wait();

    const u32 fastIterations = ttapi_dwFastIter;

    u32 iteration = 0;
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
    //steal:
    {
        task = TryToSteal();
        if (task)
            goto execute;
    }
    //count_spins:
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
    //wait:
    {
        s_tl_worker.sleeps.store(true, std::memory_order_relaxed);
        {
            s_tl_worker.event.Wait(1);
        }
        s_tl_worker.sleeps.store(false, std::memory_order_relaxed);
        iteration = 0;
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

Task* TaskManager::TryToSteal() const
{
    const auto count = workers.size();

    int steal_attempts = 5;
    while (steal_attempts > 0)
    {
        TaskWorker* other = workers[s_tl_worker.random.randI(count)];
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
    while (!task.IsFinished())
    {
        ExecuteOneTask();
        if (s_main_thread_worker == &s_tl_worker && xrDebug::ProcessingFailure())
            SDL_PumpEvents(); // Necessary to prevent dead locks
    }
}

void TaskManager::WaitForChildren(const Task& task) const
{
    while (!task.HasChildren())
    {
        ExecuteOneTask();
        if (s_main_thread_worker == &s_tl_worker && xrDebug::ProcessingFailure())
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
