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
#pragma once

#include "Task.hpp"
#include <condition_variable>

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>

class TaskWorker;

class XRCORE_API TaskManager final
{
    xr_vector<TaskWorker*> workers;
    xr_vector<std::thread> workerThreads;
    mutable std::shared_mutex workersLock;
    const size_t threadCapacity;
    std::atomic_size_t workerCount{};
    std::atomic_size_t outstandingTasks{};
    std::atomic_bool shouldPause{};
    std::atomic_bool shouldStop{};
    mutable std::mutex workMutex;
    mutable std::condition_variable workChanged;
    mutable std::atomic_uint64_t workEpoch{};

    void TaskWorkerStart();
    [[nodiscard]] Task* TryToSteal() const;
    [[nodiscard]] static Task* AllocateTask();
    static void RecycleTask(Task* task) noexcept;
    static void PushTask(Task& task) noexcept;
    static void ExecuteTask(Task& task);
    void NotifyWork(bool all) const;

    template <typename Invokable>
    static TaskHandle Schedule(Invokable&& function, Task* parent)
    {
        Task* task = AllocateTask();
        try
        {
            task->Initialize(std::move(function), parent);
        }
        catch (...)
        {
            RecycleTask(task);
            throw;
        }
        TaskHandle handle(task);
        PushTask(*task);
        return handle;
    }

public:
    TaskManager();
    ~TaskManager();

    void SpawnThreads();
    void Shutdown();
    void RegisterThisThreadAsWorker();
    void UnregisterThisThreadAsWorker();

    template <typename Invokable>
    static TaskHandle AddTask(Invokable function)
    {
        return Schedule(std::move(function), nullptr);
    }

    template <typename Invokable>
    static TaskHandle AddTask(Task& parent, Invokable function)
    {
        return Schedule(std::move(function), &parent);
    }

    void Wait(const TaskHandle& task, bool updateSystemEvents = false) const;
    bool ExecuteOneTask() const;
    void Pause(bool pause);

    [[nodiscard]] size_t GetWorkersCount() const noexcept;
    [[nodiscard]] static size_t GetCurrentWorkerID() noexcept;
    void GetStats(size_t& allocated, size_t& pushed, size_t& finished);
};

extern XRCORE_API xr_unique_ptr<TaskManager> TaskScheduler;
