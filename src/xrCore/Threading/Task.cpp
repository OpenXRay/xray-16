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
#include "stdafx.h"

#include "Task.hpp"

Task::Data::Data(pcstr name, const TaskFunc& task, Task* parent)
    : task_func(task), on_finish_callback(nullptr), name(name), parent(parent), jobs(1) {}

Task::Data::Data(pcstr name, const TaskFunc& task, const OnFinishFunc& onFinishCallback, Task* parent)
    : task_func(task), on_finish_callback(onFinishCallback), name(name), parent(parent), jobs(1) {}

Task::Task() : m_user_data() {}

Task::Task(pcstr name, const TaskFunc& task, void* data, size_t dataSize, Task* parent /*= nullptr*/)
    : m_data(name, task, parent)
{
    VERIFY2(dataSize <= sizeof(m_user_data), "Cannot fit your data in the task");
    if (data && dataSize)
    {
        CopyMemory(m_user_data, data, std::min(dataSize, sizeof(m_user_data)));
    }
}

Task::Task(pcstr name, const TaskFunc& task, const OnFinishFunc& onFinishCallback, void* data, size_t dataSize, Task* parent /*= nullptr*/)
    : m_data(name, task, onFinishCallback, parent)
{
    VERIFY2(dataSize <= sizeof(m_user_data), "Cannot fit your data in the task");
    if (data && dataSize)
    {
        CopyMemory(m_user_data, data, std::min(dataSize, sizeof(m_user_data)));
    }
}

void Task::Execute()
{
    m_data.task_func(*this, m_user_data);
}

void Task::Finish()
{
    if (m_data.on_finish_callback)
        m_data.on_finish_callback(*this, m_user_data);
}
