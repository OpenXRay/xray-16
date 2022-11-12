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

#include "Event.hpp"
#include "Task.hpp"

class TaskWorker;

class XRCORE_API TaskManager final
{
private:
    xr_vector<TaskWorker*> workers;
    Lock workersLock;

    std::atomic_size_t workersCount{};
    std::atomic_size_t activeWorkersCount{};

    std::atomic_bool shouldStop{};

    CRandom random; // non-atomic intentionally, possible data-races can make it even more random

private:
    static void task_worker_entry(void* this_ptr);
    ICN void TaskWorkerStart();

    [[nodiscard]] Task* TryToSteal(TaskWorker* thief);

    static void ExecuteTask(Task& task);
    static void FinalizeTask(Task& task);

    [[nodiscard]] ICF static Task* AllocateTask();
    static void ICF IncrementTaskJobsCounter(Task& parent);

private:
    void SetThreadStatus(bool active);
    void WakeUpIfNeeded();

public:
    TaskManager();
    ~TaskManager();

public:
    // TaskFunc is at the end for fancy in-place lambdas
    // Create a task, but don't run it yet
    [[nodiscard]] Task& CreateTask(pcstr name, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);
    [[nodiscard]] Task& CreateTask(pcstr name, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);

    // Create a task as child, but don't run it yet
    [[nodiscard]] Task& CreateTask(Task& parent, pcstr name, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);
    [[nodiscard]] Task& CreateTask(Task& parent, pcstr name, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);

    // Run task
    void PushTask(Task& task);

    // Shortcut: create a task and run it immediately
    Task& AddTask(pcstr name, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);
    Task& AddTask(pcstr name, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);

    // Shortcut: create task and run it immediately
    Task& AddTask(Task& parent, pcstr name, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);
    Task& AddTask(Task& parent, pcstr name, const Task::OnFinishFunc& onFinishCallback, const Task::TaskFunc& taskFunc, size_t dataSize = 0, void* data = nullptr);

public:
    void Wait(const Task& task);
    void WaitForChildren(const Task& task);
    bool ExecuteOneTask();

public:
    [[nodiscard]] size_t GetWorkersCount() const;
    [[nodiscard]] size_t GetActiveWorkersCount() const;
    void GetStats(size_t& allocated, size_t& allocatedWithFallback, size_t& pushed, size_t& finished);
};

extern XRCORE_API xr_unique_ptr<TaskManager> TaskScheduler;
