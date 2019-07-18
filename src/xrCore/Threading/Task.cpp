#include "stdafx.h"

#include "Task.hpp"
#include "TaskManager.hpp"

Task::Task(pcstr name, Type type, TaskFunc task, IsAllowedCallback allowed /*= nullptr*/, DoneCallback done /*= nullptr*/)
    : isExecutionAllowed(allowed), onTaskDone(done), task(task), name(name), type(type), isStarted(false)
{
    R_ASSERT2(name && xr_strlen(name) > 1, "Please, specify task name!");
}

tbb::task* Task::execute()
{
    timer.Start();
    isStarted = true;

    task();

    if (onTaskDone)
        onTaskDone();

    TaskScheduler->TaskDone(this, timer.GetElapsed_ms());

    return nullptr;
}
