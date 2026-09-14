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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

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

class XRCORE_API Task final
{
    friend class TaskAllocator;
    friend class TaskHandle;
    friend class TaskManager;

    struct Data
    {
        void (*call)(Task&) = nullptr;
        void (*destroy)(void*) noexcept = nullptr;
        Task* parent = nullptr;
        std::atomic_uint32_t jobs{};
        std::atomic_uint32_t references{};
        std::atomic_bool hasResult{};
    };

    static constexpr size_t USER_DATA_SIZE = TASK_SIZE - sizeof(Data);

    alignas(TASK_SIZE) std::byte m_user_data[USER_DATA_SIZE];
    Data m_data;

    Task() = default;

    template <typename Invokable>
    static decltype(auto) Invoke(Invokable& function, Task& task)
    {
        if constexpr (std::is_invocable_v<Invokable&, Task&>)
            return function(task);
        else
            return function();
    }

    template <typename T>
    static void Destroy(void* storage) noexcept
    {
        std::launder(static_cast<T*>(storage))->~T();
    }

    template <typename Invokable>
    static void Call(Task& task)
    {
        auto* function = std::launder(reinterpret_cast<Invokable*>(task.m_user_data));
        using Result = decltype(Invoke(*function, task));
        if constexpr (std::is_void_v<Result>)
        {
            Invoke(*function, task);
            Destroy<Invokable>(task.m_user_data);
            task.m_data.destroy = nullptr;
        }
        else
        {
            using StoredResult = std::decay_t<Result>;
            static_assert(sizeof(StoredResult) <= USER_DATA_SIZE);
            static_assert(alignof(StoredResult) <= alignof(Task));
            auto result = Invoke(*function, task);
            Destroy<Invokable>(task.m_user_data);
            task.m_data.destroy = nullptr;
            ::new (task.m_user_data) StoredResult(std::move(result));
            task.m_data.destroy = &Destroy<StoredResult>;
            task.m_data.hasResult.store(true, std::memory_order_release);
        }
    }

    template <typename Invokable>
    void Initialize(Invokable&& function, Task* parent)
    {
        static_assert(std::is_invocable_v<Invokable&> || std::is_invocable_v<Invokable&, Task&>);
        static_assert(sizeof(Invokable) <= USER_DATA_SIZE);
        static_assert(alignof(Invokable) <= alignof(Task));
        ::new (m_user_data) Invokable(std::move(function));
        m_data.call = &Call<Invokable>;
        m_data.destroy = &Destroy<Invokable>;
        m_data.parent = parent;
        m_data.jobs.store(1, std::memory_order_relaxed);
        m_data.references.store(2, std::memory_order_relaxed);
        m_data.hasResult.store(false, std::memory_order_relaxed);
        if (parent)
        {
            const auto previous = parent->m_data.jobs.fetch_add(1, std::memory_order_relaxed);
            VERIFY2(previous > 0 && previous < std::numeric_limits<uint32_t>::max(),
                "Invalid parent task completion count");
        }
    }

    void AddReference() noexcept
    {
        m_data.references.fetch_add(1, std::memory_order_relaxed);
    }

    void ReleaseReference() noexcept;
    void Finish() noexcept;

public:
    Task(Task&&) = delete;
    Task(const Task&) = delete;
    Task& operator=(Task&&) = delete;
    Task& operator=(const Task&) = delete;

    [[nodiscard]] static constexpr size_t AvailableDataStorageSize() noexcept
    {
        return USER_DATA_SIZE;
    }

    [[nodiscard]] Task* GetParent() const noexcept
    {
        return m_data.parent;
    }

    template <typename T = void>
    [[nodiscard]] const T* GetData() const noexcept
    {
        if (!m_data.hasResult.load(std::memory_order_acquire))
            return nullptr;
        if constexpr (std::is_void_v<T>)
            return m_user_data;
        else
            return std::launder(reinterpret_cast<const T*>(m_user_data));
    }

    [[nodiscard]] uint32_t GetJobsCount() const noexcept
    {
        return m_data.jobs.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool IsFinished() const noexcept
    {
        return GetJobsCount() == 0;
    }
};

class XRCORE_API TaskHandle
{
    friend class TaskManager;

    Task* m_task = nullptr;

    explicit TaskHandle(Task* task) noexcept : m_task(task) {}

public:
    TaskHandle() noexcept = default;
    TaskHandle(std::nullptr_t) noexcept {}

    TaskHandle(const TaskHandle& other) noexcept : m_task(other.m_task)
    {
        if (m_task)
            m_task->AddReference();
    }

    TaskHandle(TaskHandle&& other) noexcept : m_task(std::exchange(other.m_task, nullptr)) {}

    TaskHandle& operator=(TaskHandle other) noexcept
    {
        std::swap(m_task, other.m_task);
        return *this;
    }

    ~TaskHandle() { Reset(); }

    void Reset() noexcept
    {
        if (Task* task = std::exchange(m_task, nullptr))
            task->ReleaseReference();
    }

    explicit operator bool() const noexcept { return m_task != nullptr; }

    [[nodiscard]] bool IsFinished() const noexcept
    {
        return !m_task || m_task->IsFinished();
    }

    template <typename T = void>
    [[nodiscard]] const T* GetData() const noexcept
    {
        return m_task ? m_task->GetData<T>() : nullptr;
    }
};

static_assert(sizeof(Task) == TASK_SIZE);
