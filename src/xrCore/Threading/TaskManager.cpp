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

#include "TaskManager.hpp"
#include "Math/fast_lc16.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <random>
#include <thread>
#include <vector>
#include <SDL3/SDL_events.h>

xr_unique_ptr<TaskManager> TaskScheduler;

static constexpr size_t TASK_STORAGE_SIZE = 4096;

class TaskAllocator
{
    struct Block
    {
        Task tasks[TASK_STORAGE_SIZE];
    };

    std::mutex mutex;
    std::vector<std::unique_ptr<Block>> blocks;
    Task* available = nullptr;

public:
    static TaskAllocator& Get()
    {
        static TaskAllocator allocator;
        return allocator;
    }

    Task* Allocate()
    {
        std::lock_guard lock(mutex);
        if (!available)
        {
            auto block = std::make_unique<Block>();
            Task* tasks = block->tasks;
            blocks.push_back(std::move(block));
            for (size_t i = 0; i < TASK_STORAGE_SIZE; ++i)
            {
                tasks[i].m_data.parent = available;
                available = &tasks[i];
            }
        }
        Task* task = available;
        available = task->m_data.parent;
        return task;
    }

    void Recycle(Task* task) noexcept
    {
        std::lock_guard lock(mutex);
        task->m_data.parent = available;
        available = task;
    }
};

void Task::ReleaseReference() noexcept
{
    if (m_data.references.fetch_sub(1, std::memory_order_acq_rel) != 1)
        return;
    if (m_data.destroy)
    {
        m_data.destroy(m_user_data);
        m_data.destroy = nullptr;
    }
    TaskAllocator::Get().Recycle(this);
}

void Task::Finish() noexcept
{
    Task* task = this;
    while (task)
    {
        Task* parent = task->m_data.parent;
        if (task->m_data.jobs.fetch_sub(1, std::memory_order_acq_rel) != 1)
            break;
        task->ReleaseReference();
        task = parent;
    }
}

class TaskQueue
{
    mutable std::mutex mutex;
    xr_vector<Task*> tasks;
    size_t head = 0;
    size_t count = 0;

public:
    TaskQueue() : tasks(TASK_STORAGE_SIZE) {}

    void push(Task* task)
    {
        std::lock_guard lock(mutex);
        if (count == tasks.size())
        {
            xr_vector<Task*> grown(tasks.size() * 2);
            const size_t first = tasks.size() - head;
            std::copy_n(tasks.begin() + head, first, grown.begin());
            std::copy_n(tasks.begin(), head, grown.begin() + first);
            tasks.swap(grown);
            head = 0;
        }
        tasks[(head + count) % tasks.size()] = task;
        ++count;
    }

    Task* pop()
    {
        std::lock_guard lock(mutex);
        if (!count)
            return nullptr;
        Task* task = tasks[head];
        head = (head + 1) % tasks.size();
        --count;
        return task;
    }

    [[nodiscard]] bool empty() const
    {
        std::lock_guard lock(mutex);
        return count == 0;
    }
};

class TaskWorker : public TaskQueue
{
public:
    TaskManager* manager = nullptr;
    size_t id = size_t(-1);
    fast_lc16 random{ this };
    std::atomic_size_t allocated{};
    std::atomic_size_t pushed{};
    std::atomic_size_t finished{};
};

static thread_local TaskWorker s_tl_worker;

TaskManager::TaskManager() : threadCapacity(std::max(1u, std::thread::hardware_concurrency()))
{
    workers.reserve(threadCapacity);
    workerThreads.reserve(threadCapacity - 1);
    RegisterThisThreadAsWorker();
}

void TaskManager::SpawnThreads()
{
    R_ASSERT(s_tl_worker.manager == this && s_tl_worker.id == 0);
    R_ASSERT(workerThreads.empty());
    const size_t count = threadCapacity - GetWorkersCount();
    for (size_t i = 0; i < count; ++i)
        workerThreads.emplace_back(Threading::RunThread("Task Worker", &TaskManager::TaskWorkerStart, this));

    std::unique_lock lock(workMutex);
    workChanged.wait(lock, [this] { return GetWorkersCount() == threadCapacity; });
}

TaskManager::~TaskManager()
{
    Shutdown();
}

void TaskManager::Shutdown()
{
    if (shouldStop.load(std::memory_order_acquire))
        return;
    R_ASSERT(s_tl_worker.manager == this && s_tl_worker.id == 0);
    Pause(false);
    while (outstandingTasks.load(std::memory_order_acquire) != 0)
    {
        const auto epoch = workEpoch.load(std::memory_order_acquire);
        if (ExecuteOneTask())
            continue;
        std::unique_lock lock(workMutex);
        workChanged.wait(lock, [this, epoch]
        {
            return outstandingTasks.load(std::memory_order_acquire) == 0 ||
                workEpoch.load(std::memory_order_acquire) != epoch;
        });
    }
    shouldStop.store(true, std::memory_order_release);
    NotifyWork(true);
    for (auto& thread : workerThreads)
        thread.join();
    UnregisterThisThreadAsWorker();
}

void TaskManager::NotifyWork(bool all) const
{
    {
        std::lock_guard lock(workMutex);
        workEpoch.fetch_add(1, std::memory_order_release);
    }
    if (all || shouldPause.load(std::memory_order_acquire))
        workChanged.notify_all();
    else
        workChanged.notify_one();
}

void TaskManager::RegisterThisThreadAsWorker()
{
    R_ASSERT(!s_tl_worker.manager);
    {
        std::unique_lock lock(workersLock);
        R_ASSERT(workers.size() < threadCapacity);
        s_tl_worker.id = workers.size();
        s_tl_worker.manager = this;
        s_tl_worker.allocated.store(0, std::memory_order_relaxed);
        s_tl_worker.pushed.store(0, std::memory_order_relaxed);
        s_tl_worker.finished.store(0, std::memory_order_relaxed);
        workers.push_back(&s_tl_worker);
        workerCount.store(workers.size(), std::memory_order_release);
    }
    NotifyWork(true);
}

void TaskManager::UnregisterThisThreadAsWorker()
{
    R_ASSERT(s_tl_worker.manager == this);
    {
        std::unique_lock lock(workersLock);
        R_ASSERT(s_tl_worker.empty());
        const auto worker = std::find(workers.begin(), workers.end(), &s_tl_worker);
        R_ASSERT(worker != workers.end());
        workers.erase(worker);
        s_tl_worker.id = size_t(-1);
        s_tl_worker.manager = nullptr;
        workerCount.store(workers.size(), std::memory_order_release);
    }
    NotifyWork(true);
}

void TaskManager::TaskWorkerStart()
{
    RegisterThisThreadAsWorker();
    while (!shouldStop.load(std::memory_order_acquire))
    {
        const auto epoch = workEpoch.load(std::memory_order_acquire);
        if (!shouldPause.load(std::memory_order_acquire) && ExecuteOneTask())
            continue;
        std::unique_lock lock(workMutex);
        workChanged.wait(lock, [this, epoch]
        {
            return shouldStop.load(std::memory_order_acquire) ||
                workEpoch.load(std::memory_order_acquire) != epoch;
        });
    }
    UnregisterThisThreadAsWorker();
}

Task* TaskManager::TryToSteal() const
{
    std::shared_lock lock(workersLock);
    const size_t count = workers.size();
    if (count <= 1)
        return nullptr;
    const auto steal = [this](TaskWorker* worker) -> Task*
    {
        if (worker == &s_tl_worker)
            return nullptr;
        Task* task = worker->pop();
        if (task && !worker->empty())
            NotifyWork(false);
        return task;
    };
    if (Task* task = steal(workers.front()))
        return task;
    const size_t start = std::uniform_int_distribution<size_t>(0, count - 1)(s_tl_worker.random);
    for (size_t offset = 0; offset < count; ++offset)
    {
        const size_t index = (start + offset) % count;
        if (index != 0)
        {
            if (Task* task = steal(workers[index]))
                return task;
        }
    }
    return nullptr;
}

Task* TaskManager::AllocateTask()
{
    R_ASSERT(s_tl_worker.manager);
    s_tl_worker.allocated.fetch_add(1, std::memory_order_relaxed);
    return TaskAllocator::Get().Allocate();
}

void TaskManager::RecycleTask(Task* task) noexcept
{
    TaskAllocator::Get().Recycle(task);
}

void TaskManager::PushTask(Task& task) noexcept
{
    TaskManager& manager = *s_tl_worker.manager;
    manager.outstandingTasks.fetch_add(1, std::memory_order_relaxed);
    s_tl_worker.push(&task);
    s_tl_worker.pushed.fetch_add(1, std::memory_order_relaxed);
    manager.NotifyWork(false);
}

void TaskManager::ExecuteTask(Task& task)
{
    TaskManager& manager = *s_tl_worker.manager;
    const auto finish = [&task, &manager]
    {
        task.Finish();
        s_tl_worker.finished.fetch_add(1, std::memory_order_relaxed);
        manager.outstandingTasks.fetch_sub(1, std::memory_order_acq_rel);
        manager.NotifyWork(true);
    };
    try
    {
        task.m_data.call(task);
    }
    catch (...)
    {
        finish();
        throw;
    }
    finish();
}

void TaskManager::Wait(const TaskHandle& task, bool updateSystemEvents) const
{
    R_ASSERT(s_tl_worker.manager == this);
    while (!task.IsFinished())
    {
        const auto epoch = workEpoch.load(std::memory_order_acquire);
        const bool executed = ExecuteOneTask();
        if (s_tl_worker.id == 0 && (xrDebug::ProcessingFailure() || updateSystemEvents))
            SDL_PumpEvents();
        if (executed)
            continue;
        std::unique_lock lock(workMutex);
        const auto ready = [this, &task, epoch]
        {
            return task.IsFinished() || workEpoch.load(std::memory_order_acquire) != epoch;
        };
        if (s_tl_worker.id == 0)
            workChanged.wait_for(lock, std::chrono::milliseconds(1), ready);
        else
            workChanged.wait(lock, ready);
    }
}

bool TaskManager::ExecuteOneTask() const
{
    R_ASSERT(s_tl_worker.manager == this);
    Task* task = s_tl_worker.pop();
    if (!task)
        task = TryToSteal();
    if (!task)
        return false;
    ExecuteTask(*task);
    return true;
}

void TaskManager::Pause(bool pause)
{
    shouldPause.store(pause, std::memory_order_release);
    NotifyWork(true);
}

size_t TaskManager::GetWorkersCount() const noexcept
{
    return workerCount.load(std::memory_order_acquire);
}

size_t TaskManager::GetCurrentWorkerID() noexcept
{
    return s_tl_worker.id;
}

void TaskManager::GetStats(size_t& allocated, size_t& pushed, size_t& finished)
{
    allocated = pushed = finished = 0;
    std::shared_lock lock(workersLock);
    for (const auto* worker : workers)
    {
        allocated += worker->allocated.load(std::memory_order_relaxed);
        pushed += worker->pushed.load(std::memory_order_relaxed);
        finished += worker->finished.load(std::memory_order_relaxed);
    }
}
