#pragma once

#include "xrCore/xrDelegate/xrDelegate.h"
#include "xrCore/FTimer.h"

#include "Event.hpp"

#ifdef USE_TBB_PARALLEL
#include <tbb/task.h>
#else // hack
namespace tbb
{
class task
{
public:
    virtual ~task() = default;
    virtual task* execute() = 0;
};
}
#endif

class XRCORE_API Task : public tbb::task
{
public:
    using IsAllowedCallback = xrDelegate<bool()>;
    using DoneCallback = xrDelegate<void()>;
    using TaskFunc = xrDelegate<void()>;

private:
    friend class TaskManagerBase;
    IsAllowedCallback isExecutionAllowed;
    DoneCallback onTaskDone;
    Event* onTaskDoneEvent;
    TaskFunc task;
    CTimer timer;

    pcstr name;
    bool isStarted;

public:
    Task(pcstr name, TaskFunc&& task, IsAllowedCallback&& allowed = nullptr,
        DoneCallback&& done = nullptr, Event* doneEvent = nullptr);

    pcstr GetName() const
    {
        return name;
    }

    bool IsStarted() const
    {
        return isStarted;
    }

    virtual bool IsComplex()
    {
        return false;
    }

    bool CheckIfExecutionAllowed() const
    {
        if (isExecutionAllowed)
            return isExecutionAllowed();
        return true;
    }

    auto GetElapsedMs() const
    {
        return timer.GetElapsed_ms();
    }

    tbb::task* execute() override;
};
