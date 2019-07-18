#pragma once

#include "xrCore/xrDelegate/xrDelegate.h"
#include "xrCore/FTimer.h"

#include <tbb/task.h>

class XRCORE_API Task : public tbb::task
{
public:
    enum class Type
    {
        AI,
        Core,
        Collision,
        Engine,
        Game,
        GameSpy,
        Network,
        Particles,
        Physics,
        Renderer,
        Scripting,
        Sound,
        UI,
        Other
    };

    using IsAllowedCallback = xrDelegate<bool()>;
    using DoneCallback = xrDelegate<void()>;
    using TaskFunc = xrDelegate<void()>;

private:
    IsAllowedCallback isExecutionAllowed;
    DoneCallback onTaskDone;
    TaskFunc task;
    CTimer timer;

    pcstr name;
    Type type;
    bool isStarted;

public:
    Task(pcstr name, Type type, TaskFunc task, IsAllowedCallback allowed = nullptr, DoneCallback done = nullptr);

    pcstr GetName() const
    {
        return name;
    }

    Type GetType() const
    {
        return type;
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
