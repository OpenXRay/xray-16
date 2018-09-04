#pragma once

#include "Common/Noncopyable.hpp"
#include "_types.h"
#include "xrCommon/xr_vector.h"
#include "_math.h"
#include "log.h"
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
protected:
    u64 startTime;
    u64 pauseDuration;
    u64 pauseAccum;
    bool paused;

public:
    constexpr CTimerBase() noexcept : startTime(0), pauseDuration(0), pauseAccum(0), paused(false) {}

    ICF void Start()
    {
        if (paused)
            return;
        startTime = CPU::QPC() - pauseAccum;
    }
    ICF u64 GetElapsed_ticks() const
    {
        if (paused)
            return pauseDuration;
        else
            return CPU::QPC() - startTime - CPU::qpc_overhead - pauseAccum;
    }
    IC u32 GetElapsed_ms() const { return u32(GetElapsed_ticks() * u64(1000) / CPU::qpc_freq); }
    IC float GetElapsed_sec() const
    {
#ifndef _EDITOR
        FPU::m64r();
#endif
        float _result = float(double(GetElapsed_ticks()) / double(CPU::qpc_freq));
#ifndef _EDITOR
        FPU::m24r();
#endif
        return _result;
    }
    IC void Dump() const { Msg("* Elapsed time (sec): %f", GetElapsed_sec()); }
};

class XRCORE_API CTimer : public CTimerBase
{
    using inherited = CTimerBase;

    float m_time_factor;
    u64 m_real_ticks;
    u64 m_ticks;

    IC u64 GetElapsed_ticks(const u64& current_ticks) const
    {
        u64 delta = current_ticks - m_real_ticks;
        double delta_d = (double)delta;
        double time_factor_d = time_factor();
        double time = delta_d * time_factor_d + .5;
        u64 result = (u64)time;
        return (m_ticks + result);
    }

public:
    constexpr CTimer() noexcept : m_time_factor(1.f), m_real_ticks(0), m_ticks(0) {}
    ICF void Start() noexcept
    {
        if (paused)
            return;

        inherited::Start();
        m_real_ticks = 0;
        m_ticks = 0;
    }

    float time_factor() const noexcept { return m_time_factor; }
    void time_factor(const float time_factor) noexcept
    {
        u64 current = inherited::GetElapsed_ticks();
        m_ticks = GetElapsed_ticks(current);
        m_real_ticks = current;
        m_time_factor = time_factor;
    }

    u64 GetElapsed_ticks() const
    {
#ifndef _EDITOR
        FPU::m64r();
#endif // _EDITOR

        u64 result = GetElapsed_ticks(inherited::GetElapsed_ticks());

#ifndef _EDITOR
        FPU::m24r();
#endif // _EDITOR

        return (result);
    }

    IC u32 GetElapsed_ms() const { return (u32(GetElapsed_ticks() * u64(1000) / CPU::qpc_freq)); }
    IC float GetElapsed_sec() const
    {
#ifndef _EDITOR
        FPU::m64r();
#endif
        float result = float(double(GetElapsed_ticks()) / double(CPU::qpc_freq));
#ifndef _EDITOR
        FPU::m24r();
#endif
        return (result);
    }

    void Dump() const { Msg("* Elapsed time (sec): %f", GetElapsed_sec()); }
};

class XRCORE_API CTimer_paused_ex : public CTimer
{
    u64 save_clock;

public:
    CTimer_paused_ex() noexcept : save_clock() {}
    virtual ~CTimer_paused_ex() {}
    bool Paused() const noexcept { return paused; }
    void Pause(const bool b) noexcept
    {
        if (paused == b)
            return;

        u64 _current = CPU::QPC() - CPU::qpc_overhead;
        if (b)
        {
            save_clock = _current;
            pauseDuration = CTimerBase::GetElapsed_ticks();
        }
        else
        {
            pauseAccum += _current - save_clock;
        }
        paused = b;
    }
};

class XRCORE_API CTimer_paused : public CTimer_paused_ex
{
public:
    CTimer_paused() { g_pauseMngr().Register(*this); }
    virtual ~CTimer_paused() { g_pauseMngr().UnRegister(*this); }
};

extern XRCORE_API bool g_bEnableStatGather;
class XRCORE_API CStatTimer
{
public:
    CTimer T;
    u64 accum;
    float result;
    u32 count;

    CStatTimer();
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
        accum += T.GetElapsed_ticks();
    }

    ICF u64 GetElapsed_ticks() const { return accum; }
    IC u32 GetElapsed_ms() const { return u32(GetElapsed_ticks() * u64(1000) / CPU::qpc_freq); }
    IC float GetElapsed_sec() const
    {
#ifndef _EDITOR
        FPU::m64r();
#endif
        float _result = float(double(GetElapsed_ticks()) / double(CPU::qpc_freq));
#ifndef _EDITOR
        FPU::m24r();
#endif
        return _result;
    }
};
