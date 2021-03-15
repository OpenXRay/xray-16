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

Task::Task() : m_jobs(0) /* intentionally doesn't initialize other objects */ {}

Task::Task(pcstr name, const TaskFunc& task, void* data, size_t dataSize, Task* parent /*= nullptr*/)
    : m_task_func(task), m_name(name), m_parent(parent), m_jobs(1)
{
    VERIFY2(dataSize <= sizeof(m_data), "Cannot fit your data in the task");
    CopyMemory(&m_data, data, std::min(dataSize, sizeof(m_data)));
}

Task::Task(pcstr name, const TaskFunc& task, const OnFinishFunc& onFinishCallback, void* data, size_t dataSize, Task* parent /*= nullptr*/)
    : m_task_func(task), m_on_finish_callback(onFinishCallback), m_name(name), m_parent(parent), m_jobs(1)
{
    VERIFY2(dataSize <= sizeof(m_data), "Cannot fit your data in the task");
    CopyMemory(&m_data, data, std::min(dataSize, sizeof(m_data)));
}

void Task::Execute()
{
    m_task_func(*this, m_data);
}

void Task::Finish()
{
    if (m_on_finish_callback)
        m_on_finish_callback(*this, &m_data);
}
