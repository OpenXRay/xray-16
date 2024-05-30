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

#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_PPC)
constexpr size_t RECOMMENDED_TASK_SIZE = 64; // bytes
#elif defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_ARM64) || defined(XR_ARCHITECTURE_E2K) || defined(XR_ARCHITECTURE_PPC64)
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

private:
    using CallFunc  = void(Task&);

    // ordered from biggest to smallest
    struct Data
    {
        CallFunc*           call;
        Task*               parent;
        std::atomic_int16_t jobs; // at least 1 (task itself), zero means task is done.

        Data() = default;
        Data(CallFunc* call, Task* parent)
            : call(call), parent(parent), jobs(1) {}
    };

    static constexpr size_t USER_DATA_SIZE = TASK_SIZE - sizeof(Data);

    std::byte m_user_data[USER_DATA_SIZE];
    Data      m_data;

    template <typename Invokable,
        bool TaskAware = std::is_invocable_v<Invokable, Task&>,
        typename HasResult = void
    >
    struct Dispatcher
    {
        static constexpr bool task_unaware = std::is_invocable_v<Invokable>;

        static_assert(TaskAware || task_unaware,
            "Provide callable with type: void() or void(Task&) or T() or T(Task&), "
            "where T is any result of your function.");
    };

    template <typename Invokable>
    struct Dispatcher<Invokable, true, std::enable_if_t<std::is_same_v<void, std::invoke_result_t<Invokable, Task&>>>>
    {
        static void Call(Task& task)
        {
            auto& obj = *reinterpret_cast<Invokable*>(task.m_user_data);
            obj(task);
            if constexpr (!std::is_trivially_copyable_v<Invokable>)
                obj.~Invokable();
        }
    };

    template <typename Invokable>
    struct Dispatcher<Invokable, false, std::enable_if_t<std::is_same_v<void, std::invoke_result_t<Invokable>>>>
    {
        static void Call(Task& task)
        {
            auto& obj = *reinterpret_cast<Invokable*>(task.m_user_data);
            obj();
            if constexpr (!std::is_trivially_copyable_v<Invokable>)
                obj.~Invokable();
        }
    };

    template <typename Invokable>
    struct Dispatcher<Invokable, true, std::enable_if_t<!std::is_same_v<void, std::invoke_result_t<Invokable, Task&>>>>
    {
        static_assert(sizeof(std::invoke_result_t<Invokable>) <= USER_DATA_SIZE,
            "Not enough storage to save result of your function. Try to reduce its size.");

        static void Call(Task& task)
        {
            auto& obj = *reinterpret_cast<Invokable*>(task.m_user_data);
            auto result = std::move(obj(task));
            if constexpr (!std::is_trivially_copyable_v<Invokable>)
                obj.~Invokable();
            ::new (task.m_user_data) decltype(result)(std::move(result));
        }
    };

    template <typename Invokable>
    struct Dispatcher<Invokable, false, std::enable_if_t<!std::is_same_v<void, std::invoke_result_t<Invokable>>>>
    {
        static_assert(sizeof(std::invoke_result_t<Invokable>) <= USER_DATA_SIZE,
            "Not enough storage to save result of your function. Try to reduce its size.");

        static void Call(Task& task)
        {
            auto& obj = *reinterpret_cast<Invokable*>(task.m_user_data);
            auto result = std::move(obj());
            if constexpr (!std::is_trivially_copyable_v<Invokable>)
                obj.~Invokable();
            ::new (task.m_user_data) decltype(result)(std::move(result));
        }
    };

private:
    Task() = default; // used by TaskAllocator as Task initial state

    template <typename Invokable>
    Task(Invokable func, Task* parent = nullptr)
        : m_data(&Dispatcher<Invokable>::Call, parent)
    {
        static_assert(sizeof(Invokable) <= USER_DATA_SIZE,
            "Not enough storage to save your functor/lambda. Try to reduce its size.");

        if constexpr (!std::is_empty_v<Invokable> || !std::is_trivially_copyable_v<Invokable>)
        {
            ::new (m_user_data) Invokable(std::move(func));
        }

        if (parent)
        {
            VERIFY2(parent->m_data.jobs.load(std::memory_order_relaxed) > 0, "Adding child task to a parent that has already finished.");
            [[maybe_unused]] const auto prev = parent->m_data.jobs.fetch_add(1, std::memory_order_acq_rel);
            VERIFY2(prev != std::numeric_limits<decltype(prev)>::max(), "Max jobs overflow. (too much children)");
        }
    }

public:
    Task(Task&&) = delete;
    Task(const Task&) = delete;
    Task& operator=(Task&&) = delete;
    Task& operator=(const Task&) = delete;

    [[nodiscard]]
    static constexpr size_t AvailableDataStorageSize() noexcept
    {
        return sizeof(m_user_data);
    }

    [[nodiscard]]
    auto GetParent() const noexcept
    {
        return m_data.parent;
    }

    template <typename T = void>
    [[nodiscard]]
    const T* GetData() const noexcept
    {
        if (!IsFinished())
            return nullptr;
        return reinterpret_cast<const T*>(m_user_data);
    }

    [[nodiscard]]
    auto GetJobsCount() const noexcept
    {
        return m_data.jobs.load(std::memory_order_relaxed);
    }

    [[nodiscard]]
    bool IsFinished() const noexcept
    {
        return GetJobsCount() == 0;
    }

private:
    // Called by TaskManager
    void operator()()
    {
        m_data.call(*this);

        for (Task* it = this; ; it = it->m_data.parent)
        {
            const auto unfinishedJobs = it->m_data.jobs.fetch_sub(1, std::memory_order_acq_rel) - 1; // fetch_sub returns previous value
            VERIFY2(unfinishedJobs >= 0, "The same task was executed two times.");
            if (unfinishedJobs || !it->m_data.parent)
                break;
        }
    }
};

static_assert(sizeof(Task) == TASK_SIZE);
