#pragma once

#include "Common/Noncopyable.hpp"
#include "_types.h"
#include "xrCommon/xr_vector.h"
#include "_math.h"
#include "log.h"
#include "Threading/ScopeLock.hpp"

#include <chrono>

class CTimer_paused;

class XRCORE_API pauseMngr : Noncopyable
{
    xr_vector<CTimer_paused*> m_timers;
    bool paused;

public:
    pauseMngr();
    bool Paused() const { return paused; }
    void Pause(const bool b);
    void Register(CTimer_paused& t);
    void UnRegister(CTimer_paused& t);
};

extern XRCORE_API pauseMngr& g_pauseMngr();

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
    constexpr CTimerBase() noexcept : startTime(), pauseDuration(), pauseAccum(), paused(false) {}

    ICF void Start()
    {
        if (paused)
            return;
        startTime = Now() - pauseAccum;
    }

    virtual Duration getElapsedTime() const
    {
        if (paused)
            return pauseDuration;
        return Now() - startTime - pauseAccum;
    }

    u64 GetElapsed_ns() const
    {
        using namespace std::chrono;
        return duration_cast<nanoseconds>(getElapsedTime()).count();
    }

    u64 GetElapsed_ms() const
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(getElapsedTime()).count();
    }

    IC float GetElapsed_sec() const
    {
        using namespace std::chrono;
        return duration_cast<duration<float>>(getElapsedTime()).count();
    }

    Time Now() const { return Clock::now(); }

    IC void Dump() const { Msg("* Elapsed time (sec): %f", GetElapsed_sec()); }
};

class XRCORE_API CTimer : public CTimerBase
{
    using inherited = CTimerBase;

    float m_time_factor;
    Duration realTime;
    Duration time;

    inline Duration getElapsedTime(const Duration& current) const
    {
        const auto delta = current - realTime;
        const double deltaD = double(delta.count());
        const double elapsedTime = deltaD * m_time_factor + .5;
        const auto result = u64(elapsedTime);
        return Duration(this->time.count() + result);
    }

public:
    constexpr CTimer() noexcept : m_time_factor(1.f), realTime(0), time(0) {}

    void Start() noexcept
    {
        if (paused)
            return;

        realTime = std::chrono::nanoseconds(0);
        time = std::chrono::nanoseconds(0);
        inherited::Start();
    }

    float time_factor() const noexcept { return m_time_factor; }
    void time_factor(const float time_factor) noexcept
    {
        const Duration current = inherited::getElapsedTime();
        time = getElapsedTime(current);
        realTime = current;
        m_time_factor = time_factor;
    }

    Duration getElapsedTime() const override
    {
        return getElapsedTime(inherited::getElapsedTime());
    }
};

class XRCORE_API CTimer_paused_ex : public CTimer
{
    Time save_clock;

public:
    CTimer_paused_ex() noexcept : save_clock() {}
    virtual ~CTimer_paused_ex() = default;
    bool Paused() const noexcept { return paused; }
    void Pause(const bool b) noexcept
    {
        if (paused == b)
            return;

        const auto current = Now();
        if (b)
        {
            save_clock = current;
            pauseDuration = CTimerBase::getElapsedTime();
        }
        else
        {
            pauseAccum += current - save_clock;
        }
        paused = b;
    }
};

class XRCORE_API CTimer_paused final : public CTimer_paused_ex
{
public:
    CTimer_paused() { g_pauseMngr().Register(*this); }
    ~CTimer_paused() override { g_pauseMngr().UnRegister(*this); }
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

    ICF void Begin()
    {
        if (!g_bEnableStatGather)
            return;
        count++;
        T.Start();
    }

    ICF void End()
    {
        if (!g_bEnableStatGather)
            return;
        accum += T.getElapsedTime();
    }

    // Instead of making the entire timer thread-safe,
    // we can create stat. timers on stack and append their results
    // to the main timer
    // Takes external lock because not every timer should be multi-threaded.
    void AppendResults(Lock& lock, const CStatTimer& other) // thread-safe
    {
        if (!g_bEnableStatGather)
            return;
        ScopeLock scope(&lock);
        VERIFY2(fis_zero(other.result), "Appended timer is supposed to not have frame result.");
        accum += other.accum;
        count += other.count;
    }

    Duration getElapsedTime() const { return accum; }

    u64 GetElapsed_ns() const
    {
        using namespace std::chrono;
        return duration_cast<nanoseconds>(getElapsedTime()).count();
    }

    u64 GetElapsed_ms() const
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(getElapsedTime()).count();
    }

    float GetElapsed_sec() const
    {
        using namespace std::chrono;
        return duration_cast<duration<float>>(getElapsedTime()).count();
    }
};

class ScopeStatTimer : public CStatTimer
{
    CStatTimer& baseTimer;
    Lock& baseTimerLock;

public:
    ScopeStatTimer(CStatTimer& base, Lock& lock) : baseTimer(base), baseTimerLock(lock)
    {
        if (!g_bEnableStatGather)
            return;
        Begin();
    }

    ~ScopeStatTimer()
    {
        if (!g_bEnableStatGather)
            return;
        End();
        baseTimer.AppendResults(baseTimerLock, *this);
    }
};
