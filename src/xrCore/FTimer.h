#pragma once
#ifndef FTimerH
#define FTimerH
#include "_types.h"
#include "xrCore_impexp.h"
#include "xrCommon/xr_vector.h"
//#include "_stl_extensions.h"
#include "_math.h"
#include "log.h"
#include <chrono>

class CTimer_paused;

class XRCORE_API pauseMngr
{
    xr_vector<CTimer_paused*> m_timers;
    bool paused;

public:
    pauseMngr();
    bool Paused() const { return paused; };
    void Pause(const bool b);
    void Register(CTimer_paused& t);
    void UnRegister(CTimer_paused& t);
};

extern XRCORE_API pauseMngr* g_pauseMngr();

class XRCORE_API CTimerBase
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using Time = std::chrono::time_point<Clock>;
    using Duration = Time::duration;

protected:
    Time startTime;
    Duration pauseDuration;
    Duration pauseAccum;
    bool paused;

public:
    CTimerBase() : startTime(), pauseDuration(), pauseAccum(), paused(false) {}

    void Start()
    {
        if (paused) return;
        startTime = Clock::now() - pauseAccum;
    }

    Duration getElapsedTime() const
    {
        if (paused) return pauseDuration;
        return Clock::now() - startTime - pauseAccum;
    }

    u64 GetElapsed_ms() const
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(getElapsedTime()).count();
    }

    float GetElapsed_sec() const
    {
        using namespace std::chrono;
        const auto nanos = duration_cast<nanoseconds>(getElapsedTime()).count();
        return float(nanos) / 1000000000.0;
    }

    void Dump() const { Msg("* Elapsed time (sec): %f", GetElapsed_sec()); }
};

class XRCORE_API CTimer : public CTimerBase
{
    using super = CTimerBase;

    float m_time_factor;
    Duration realTime;
    Duration time;

    Duration getElapsedTime(const Duration current) const
    {
        const auto delta = current - realTime;
        const auto deltaD = double(delta.count());
        const auto time_factor_d = double(time_factor());
        const double time = deltaD * time_factor_d + .5;
        const auto result = u64(time);
        return Duration(result);
    }

public:
    CTimer() : m_time_factor(1.f), realTime(), time() {}

    void Start()
    {
        if (paused) return;

        super::Start();
    }

    float time_factor() const { return m_time_factor; }

    void time_factor(const float time_factor)
    {
        const auto current = super::getElapsedTime();
        time = getElapsedTime(current);
        realTime = current;
        m_time_factor = time_factor;
    }

    Duration getElapsedTime() const
    {
        return super::getElapsedTime();
    }
};

class XRCORE_API CTimer_paused_ex : public CTimer
{
    Time save_clock;

public:
    CTimer_paused_ex() : save_clock() {}
    virtual ~CTimer_paused_ex() {}
    bool Paused() const { return paused; }

    void Pause(const bool b)
    {
        if (paused == b) return;

        const auto current = Clock::now();
        if (b)
        {
            save_clock = current;
            pauseDuration = CTimerBase::getElapsedTime();
        }
        else
            pauseAccum += current - save_clock;

        paused = b;
    }
};

class XRCORE_API CTimer_paused : public CTimer_paused_ex
{
public:
    CTimer_paused() { g_pauseMngr()->Register(*this); }
    virtual ~CTimer_paused() { g_pauseMngr()->UnRegister(*this); }
};

extern XRCORE_API bool g_bEnableStatGather;

class XRCORE_API CStatTimer
{
    using Duration = CTimerBase::Duration;

public:
    CTimer T;
    Duration accum;
    float result;
    u32 count;

    CStatTimer() : T(), accum(), result(.0f), count(0) {}
    void FrameStart();
    void FrameEnd();

    void Begin()
    {
        if (!g_bEnableStatGather)
            return;
        count++;
        T.Start();
    }

    void End()
    {
        if (!g_bEnableStatGather)
            return;
        accum += T.getElapsedTime();
    }

    Duration getElapsedTime() const { return accum; }

    u64 GetElapsed_ms() const
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(getElapsedTime()).count();
    }

    float GetElapsed_sec() const
    {
        using namespace std::chrono;
        const auto nanos = duration_cast<nanoseconds>(getElapsedTime()).count();
        return float(nanos) / 1000000000.0;
    }
};

#endif // FTimerH
