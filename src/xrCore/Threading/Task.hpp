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

#include <new> // for std::hardware_destructive_interference_size

#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_ARM)
constexpr size_t RECOMMENDED_TASK_SIZE = 64; // bytes
#elif defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_ARM64) || defined(XR_ARCHITECTURE_E2K)
constexpr size_t RECOMMENDED_TASK_SIZE = 128; // bytes
#else
#error Determine your platform requirements
#endif

// Use hardware_destructive_interference_size if it is implemented
// Note: libc++ 8.0 and later define __cpp_lib_hardware_interference_size but don't actually implement it
// XXX: check for particular libc++ version, when interference size support will be implemented
#if defined(__cpp_lib_hardware_interference_size) && !defined(_LIBCPP_VERSION)
constexpr size_t TASK_SIZE = std::max(RECOMMENDED_TASK_SIZE, std::hardware_destructive_interference_size);
#else
constexpr size_t TASK_SIZE = RECOMMENDED_TASK_SIZE;
#endif

class XRCORE_API Task final : Noncopyable
{
    friend class TaskManager;
    friend class TaskAllocator;
    friend class FallbackTaskAllocator;

public:
    using TaskFunc      = fastdelegate::FastDelegate<void(Task&, void*)>;
    using OnFinishFunc  = fastdelegate::FastDelegate<void(const Task&, void*)>;

private:
    // ordered from biggest to smallest
    struct Data
    {
        TaskFunc            task_func{};
        OnFinishFunc        on_finish_callback{};
        pcstr               name{};
        Task*               parent{};
        std::atomic_int16_t jobs{}; // at least 1 (task itself), zero means task is done.

        Data() = default;
        Data(pcstr name, const TaskFunc& task, Task* parent);
        Data(pcstr name, const TaskFunc& task, const OnFinishFunc& onFinishCallback, Task* parent);
    } m_data;

    u8 m_user_data[TASK_SIZE - sizeof(m_data)];

private:
    // Used by TaskAllocator as Task initial state
    Task();

    // Will just execute
    Task(pcstr name, const TaskFunc& task, void* data, size_t dataSize, Task* parent = nullptr);

    // Will execute and call back
    Task(pcstr name, const TaskFunc& task, const OnFinishFunc& onFinishCallback, void* data, size_t dataSize, Task* parent = nullptr);

public:
    static constexpr size_t GetAvailableDataStorageSize()
    {
        return sizeof(m_user_data);
    }

    Task* GetParent() const
    {
        return m_data.parent;
    }

    auto GetJobsCount() const
    {
        return m_data.jobs.load(std::memory_order_relaxed);
    }

    bool HasChildren() const
    {
        return GetJobsCount() > 1;
    }

    bool IsFinished() const
    {
        return 0 == m_data.jobs.load(std::memory_order_relaxed);
    }

private:
    // Called by TaskManager
    void Execute();
    void Finish();
};
