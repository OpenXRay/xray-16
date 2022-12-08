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

namespace details
{
class ParallelForEachTask
{
public:
    template <typename Iterator, typename Function, typename ThirdArgument>
    static decltype(auto) Run(Iterator begin, Iterator end, ThirdArgument thirdArgument, const Function& function)
    {
        return xr_parallel_for(TaskRange(begin, end), thirdArgument, [&](const TaskRange<Iterator>& range)
        {
            const Iterator ite = range.end();
            for (Iterator it = range.begin(); it != ite; ++it)
            {
                function(*it);
            }
        });
    }
};
} // namespace details

// User can specify if he wants caller thread to wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(const Range& range, bool wait, const Function& function)
{
    return details::ParallelForEachTask::Run(std::begin(range), std::end(range), wait, function);
}

// Caller thread will wait on the task finish
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(const Range& range, const Function& function)
{
    return details::ParallelForEachTask::Run(std::begin(range), std::end(range), true, function);
}

// User has a callback, he is responsible for waiting on the task finish (due to task management system limitation)
template <typename Range, typename Function>
decltype(auto) xr_parallel_for_each(const Range& range, const Task::OnFinishFunc& callback, const Function& function)
{
    return details::ParallelForEachTask::Run(std::begin(range), std::end(range), callback, function);
}
