#include "stdafx.h"

#include "Task.hpp"
#include "TaskManager.hpp"

Task::Task(pcstr name, TaskFunc&& task, IsAllowedCallback&& allowed /*= nullptr*/,
    DoneCallback&& done /*= nullptr*/, Event* doneEvent /*= nullptr*/)
    : isExecutionAllowed(allowed), onTaskDone(done), onTaskDoneEvent(doneEvent),
      task(task), name(name), isStarted(false)
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

    if (onTaskDoneEvent)
        onTaskDoneEvent->Set();

    TaskScheduler->TaskDone(this, timer.GetElapsed_ms());

    return nullptr;
}
