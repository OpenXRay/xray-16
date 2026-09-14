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

#include "ParallelFor.hpp"

#include <stdexcept>

namespace detail
{
class ParallelForEach
{
public:
    template <typename Iterator, typename Function>
    static TaskHandle RunAndWait(Task* parent, Iterator begin, Iterator end, Function& function)
    {
        auto invoke = [&function](TaskRange<Iterator>& range)
        {
            for (auto& it : range)
                function(it);
        };
        return parent ? xr_parallel_for(*parent, TaskRange(begin, end), invoke) :
                        xr_parallel_for(TaskRange(begin, end), invoke);
    }

    template <typename Iterator, typename Function>
    static TaskHandle Run(Task* parent, Iterator begin, Iterator end, bool wait, Function&& function)
    {
        if (wait)
            return RunAndWait(parent, begin, end, function);

        if constexpr (std::is_constructible_v<std::decay_t<Function>, Function&&>)
        {
            struct Invocation
            {
                Iterator begin;
                Iterator end;
                std::decay_t<Function> function;

                void operator()(Task& task)
                {
                    RunAndWait(&task, begin, end, function);
                }
            };
            if constexpr (sizeof(Invocation) <= Task::AvailableDataStorageSize() && alignof(Invocation) <= alignof(Task))
            {
                return parent ? TaskManager::AddTask(*parent, Invocation{ begin, end, std::forward<Function>(function) }) :
                                TaskManager::AddTask(Invocation{ begin, end, std::forward<Function>(function) });
            }
            else
                throw std::invalid_argument("Asynchronous xr_parallel_for_each callable exceeds task storage");
        }
        else
            throw std::invalid_argument("Asynchronous xr_parallel_for_each requires an ownable callable");
    }
};
} // namespace detail

// User can specify if he wants caller thread to wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(Range& range, bool wait, Function&& function)
{
    return detail::ParallelForEach::Run(nullptr, std::begin(range), std::end(range), wait, std::forward<Function>(function));
}

// Caller thread will wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(Range& range, Function&& function)
{
    return detail::ParallelForEach::RunAndWait(nullptr, std::begin(range), std::end(range), function);
}

// User can specify if he wants caller thread to wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(Task& parent, Range& range, bool wait, Function&& function)
{
    return detail::ParallelForEach::Run(&parent, std::begin(range), std::end(range), wait, std::forward<Function>(function));
}

// Caller thread will wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(Task& parent, Range& range, Function&& function)
{
    return detail::ParallelForEach::RunAndWait(&parent, std::begin(range), std::end(range), function);
}
