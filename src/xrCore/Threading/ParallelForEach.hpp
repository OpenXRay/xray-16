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

namespace detail
{
class ParallelForEach
{
public:
    template <typename Iterator, typename Function>
    static decltype(auto) Run(Iterator begin, Iterator end, bool wait, const Function& function)
    {
        return xr_parallel_for(TaskRange(begin, end), wait, [&](TaskRange<Iterator>& range)
        {
            for (auto& it : range)
            {
                function(it);
            }
        });
    }

    template <typename Iterator, typename Function>
    static decltype(auto) Run(Task& parent, Iterator begin, Iterator end, bool wait, const Function& function)
    {
        return xr_parallel_for(parent, TaskRange(begin, end), wait, [&](TaskRange<Iterator>& range)
        {
            for (auto& it : range)
            {
                function(it);
            }
        });
    }

};
} // namespace detail

// User can specify if he wants caller thread to wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(Range& range, bool wait, const Function& function)
{
    return detail::ParallelForEach::Run(std::begin(range), std::end(range), wait, function);
}

// Caller thread will wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(Range& range, const Function& function)
{
    return detail::ParallelForEach::Run(std::begin(range), std::end(range), true, function);
}

// User can specify if he wants caller thread to wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(Task& parent, Range& range, bool wait, const Function& function)
{
    return detail::ParallelForEach::Run(parent, std::begin(range), std::end(range), wait, function);
}

// Caller thread will wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(Task& parent, Range& range, const Function& function)
{
    return detail::ParallelForEach::Run(parent, std::begin(range), std::end(range), true, function);
}
