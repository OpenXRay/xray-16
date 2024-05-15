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

#include <mutex>

#include "Event.hpp"
#include "Task.hpp"

class TaskWorker;

class XRCORE_API TaskManager final
{
private:
    xr_vector<TaskWorker*> workers;
    xr_vector<std::thread> workerThreads;
    std::mutex workersLock;

    inline static std::condition_variable newWorkArrived;
    std::atomic_size_t activeWorkersCount{};

    std::atomic_bool shouldPause{};
    std::atomic_bool shouldStop{};

private:
    ICN void TaskWorkerStart();

    [[nodiscard]] Task* TryToSteal() const;

    static void ExecuteTask(Task& task);

    [[nodiscard]] ICF static Task* AllocateTask();
    static void ICF IncrementTaskJobsCounter(Task& parent);

    void SetThreadStatus(bool active);

public:
    TaskManager();
    ~TaskManager();
    void SpawnThreads();

public:
    // TaskFunc is at the end for fancy in-place lambdas
    // Create a task, but don't run it yet
    [[nodiscard]] static Task& CreateTask(const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);

    // Create a task as child, but don't run it yet
    [[nodiscard]] static Task& CreateTask(Task& parent, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);

    // Run task in parallel
    static void PushTask(Task& task);

    // Run task immediately in this thread
    static void RunTask(Task& task);

    // Shortcut: create a task and run it immediately
    static Task& AddTask(const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);

    // Shortcut: create task and run it immediately
    static Task& AddTask(Task& parent, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);

public:
    void RegisterThisThreadAsWorker();
    void UnregisterThisThreadAsWorker();

    void Wait(const Task& task) const;
    bool ExecuteOneTask() const;

    void Pause(bool pause) { shouldPause.store(pause, std::memory_order_release); }

public:
    [[nodiscard]] size_t GetWorkersCount() const;
    [[nodiscard]] static size_t GetCurrentWorkerID();
    void GetStats(size_t& allocated, size_t& pushed, size_t& finished);
};

extern XRCORE_API xr_unique_ptr<TaskManager> TaskScheduler;
