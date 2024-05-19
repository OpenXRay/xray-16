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

static constexpr size_t OTHER_THREADS_COUNT = 1; // Primary and Secondary thread

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
    Task* allocate() noexcept
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
    void push(Task* task) noexcept
    {
        const auto task_pos = m_tail_pos.fetch_add(1, std::memory_order_acq_rel);
        VERIFY2(task_pos - m_head_pos.load(std::memory_order_acquire) < TASK_STORAGE_SIZE, "Task queue overflow");
        m_storage[task_pos & TASK_STORAGE_MASK] = task;
    }

    Task* pop() noexcept
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

    Task* steal() noexcept
    {
        return pop();
    }

    size_t size() const noexcept
    {
        return m_head_pos - m_tail_pos;
    }

    bool empty() const noexcept
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
    std::mutex       mutex;
    fast_lc16        random{ this };
    size_t           id    { size_t(-1) };
} static thread_local s_tl_worker;

TaskManager::TaskManager()
{
    ZoneScoped;
    workers.reserve(std::thread::hardware_concurrency());
    RegisterThisThreadAsWorker();
}

void TaskManager::SpawnThreads()
{
    ZoneScoped;

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
    newWorkArrived.notify_all();
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

void TaskManager::SetThreadStatus(bool active) noexcept
{
    if (active)
        activeWorkersCount.fetch_add(1, std::memory_order_relaxed);
    else
        activeWorkersCount.fetch_sub(1, std::memory_order_relaxed);
}

void TaskManager::TaskWorkerStart()
{
    RegisterThisThreadAsWorker();
    SetThreadStatus(true);

    while (true)
    {
        if (shouldStop.load(std::memory_order_consume))
            break;

        if (!shouldPause.load(std::memory_order_consume))
        {
            if (ExecuteOneTask())
                continue;
        }

        SetThreadStatus(false);
        {
            std::unique_lock lck(s_tl_worker.mutex);
            newWorkArrived.wait(lck, [&]
            {
                // spurious unlocks allowed
                return !shouldPause.load(std::memory_order_consume);
            });
        }
        SetThreadStatus(true);
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
            if (!other->empty())
                newWorkArrived.notify_all();
            return task;
        }
        --steal_attempts;
    }
    return nullptr;
}

Task* TaskManager::AllocateTask() noexcept
{
    ++s_tl_worker.allocatedTasks;
    return s_tl_allocator.allocate();
}

void TaskManager::PushTask(Task& task) noexcept
{
    s_tl_worker.push(&task);
    newWorkArrived.notify_one();
    ++s_tl_worker.pushedTasks;
}

void TaskManager::ExecuteTask(Task& task)
{
    task();
    ++s_tl_worker.finishedTasks;
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

bool TaskManager::ExecuteOneTask() const
{
    Task* task = s_tl_worker.pop();

    if (!task)
        task = workers[0]->steal();

    if (!task)
        task = TryToSteal();

    if (task)
    {
        ExecuteTask(*task);
        return true;
    }
    return false;
}

size_t TaskManager::GetWorkersCount() const noexcept
{
    return workers.size();
}

size_t TaskManager::GetCurrentWorkerID() noexcept
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
