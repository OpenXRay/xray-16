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

#include "TaskManager.hpp"

class TaskWorker;

enum class SplitTaskRange {};

template <typename T>
class TaskRange
{
public:
    using this_type = TaskRange<T>;

    // For compatibility with STL
    using iterator = T;
    using const_iterator = T;
    using reverse_iterator = std::reverse_iterator<const_iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using size_type = size_t;

public:
    constexpr TaskRange() = default;
    constexpr TaskRange(T begin, T end) : m_begin(begin), m_end(end), m_grain(size() / TaskScheduler->GetWorkersCount())
    {
        if (m_grain <= 0)
            m_grain = 1;
    }

    TaskRange(T begin, T end, size_t grain) : m_begin(begin), m_end(end), m_grain(grain > 0 ? grain : 1)
    {
        VERIFY2(grain > 0, "Grain should be positive");
    }

    TaskRange(this_type& other, SplitTaskRange)
        : m_begin(other.m_begin), m_end(split(other)), m_grain(other.m_grain)
    {
        VERIFY2(m_end == other.m_begin, "range has been split incorrectly");
    }

private:
    static T split(this_type& other)
    {
        T middle;
        if constexpr (std::is_arithmetic_v<iterator>)
            middle = other.m_begin + (other.size()) / 2u;
        else
        {
            middle = std::next(other.m_begin, (other.size()) / 2u);
        }
        other.m_begin = middle;
        return middle;
    }

public:
    constexpr const_iterator begin() const noexcept
    {
        return iterator(m_begin);
    }

    constexpr const_iterator end() const noexcept
    {
        return iterator(m_end);
    }

    constexpr reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(end());
    }

    constexpr reverse_iterator rend() const noexcept
    {
        return reverse_iterator(begin());
    }

    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend() const noexcept { return end(); }

    constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    constexpr const_reverse_iterator crend() const noexcept { return rend(); }

public:
    size_type size() const noexcept
    {
        size_t size;
        if constexpr (std::is_arithmetic_v<iterator>)
            size = m_end - m_begin;
        else
        {
            size = std::distance(m_begin, m_end);
        }
        VERIFY(size >= 0);
        return size;
    }

    constexpr bool empty() const noexcept
    {
        return m_begin == m_end;
    }

    constexpr bool is_splittable() const noexcept
    {
        return m_grain < size();
    }

private:
    T m_begin{};
    T m_end{};
    size_t m_grain{ 1 };
};

namespace details
{
template <typename Range, typename Function>
class ParallelForTask
{
public:
    static decltype(auto) Run(const Range& range, bool wait, const Function& function)
    {
        TaskData taskData{ range, function };

        auto& task = TaskScheduler->AddTask(__FUNCTION__, task_func, sizeof(TaskData), &taskData);
        if (wait)
            TaskScheduler->Wait(task);
        return task;
    }

    // Doesn't wait until done
    static decltype(auto) Run(const Range& range, bool wait, const Task::OnFinishFunc& callback, const Function& function)
    {
        TaskData taskData{ range, function };

        auto& task = TaskScheduler->AddTask(__FUNCTION__, callback, task_func, sizeof(TaskData), &taskData);
        if (wait)
            TaskScheduler->Wait(task);
        return task;
    }

private:
    static void xr_stdcall task_func(Task& thisTask, void* data_ptr)
    {
        auto& data = *static_cast<TaskData*>(data_ptr);
        auto& range = data.range;

        if (range.is_splittable())
        {
            TaskData leftData{ TaskRange(range, SplitTaskRange()), data.function };
            TaskScheduler->AddTask(thisTask, __FUNCTION__, task_func, sizeof(TaskData), &leftData);
            TaskScheduler->AddTask(thisTask, __FUNCTION__, task_func, sizeof(TaskData), &data);
        }
        else
        {
            data.function(range);
        }
    }

private:
    struct TaskData
    {
        Range range;
        const Function function;
    };
};
} // namespace details

// User can specify if he wants caller thread to wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for(const Range& range, bool wait, const Function& function)
{
    return details::ParallelForTask<Range, Function>::Run(range, wait, function);
}

// Caller thread will wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for(const Range& range, const Function& function)
{
    return details::ParallelForTask<Range, Function>::Run(range, true, function);
}

template <typename Range, typename Function>
decltype(auto) xr_parallel_for(const Range& range, bool wait, const Task::OnFinishFunc& callback, const Function& function)
{
    return details::ParallelForTask<Range, Function>::Run(range, wait, callback, function);
}

// Caller thread will wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for(const Range& range, const Task::OnFinishFunc& callback, const Function& function)
{
    return details::ParallelForTask<Range, Function>::Run(range, true, callback, function);
}
