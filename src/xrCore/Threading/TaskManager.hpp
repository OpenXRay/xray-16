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
#include "Event.hpp"

#include <atomic>
#include <mutex>

class TaskWorker;

class XRCORE_API TaskManager final
{
private:
    xr_vector<TaskWorker*> workers;
    xr_vector<std::thread> workerThreads;
    std::mutex workersLock;

    inline static Event newWorkArrived;
    std::atomic_size_t activeWorkersCount{};

    std::atomic_bool shouldPause{};
    std::atomic_bool shouldStop{};

private:
    ICN void TaskWorkerStart();

    [[nodiscard]] Task* TryToSteal() const;

    [[nodiscard]] static Task* AllocateTask() noexcept;

    static void ExecuteTask(Task& task);

    void SetThreadStatus(bool active) noexcept;

public:
    TaskManager();
    ~TaskManager();

    void SpawnThreads();

    void RegisterThisThreadAsWorker();
    void UnregisterThisThreadAsWorker();

public:
    // Create a task, but don't run it yet
    template <typename Invokable>
    [[nodiscard]] static Task& CreateTask(Invokable func)
    {
        return *new (AllocateTask()) Task(func);
    }

    // Create a task as child, but don't run it yet
    template <typename Invokable>
    [[nodiscard]] static Task& CreateTask(Task& parent, Invokable func)
    {
        return *new (AllocateTask()) Task(func, &parent);
    }

    // Run task in parallel
    static void PushTask(Task& task) noexcept;

    // Run task immediately in this thread
    static void RunTask(Task& task);

    // Shortcut: create a task and run it immediately
    template <typename Invokable>
    static Task& AddTask(Invokable func)
    {
        Task& task = CreateTask(func);
        PushTask(task);
        return task;
    }

    // Shortcut: create task and run it immediately
    template <typename Invokable>
    static Task& AddTask(Task& parent, Invokable func)
    {
        Task& task = CreateTask(parent, func);
        PushTask(task);
        return task;
    }

public:
    void Wait(const Task& task, bool updateSystemEvents = false) const;
    bool ExecuteOneTask() const;

    void Pause(bool pause) { shouldPause.store(pause, std::memory_order_release); }

public:
    [[nodiscard]] size_t GetWorkersCount() const noexcept;
    [[nodiscard]] static size_t GetCurrentWorkerID() noexcept;
    void GetStats(size_t& allocated, size_t& pushed, size_t& finished);
};

extern XRCORE_API xr_unique_ptr<TaskManager> TaskScheduler;
