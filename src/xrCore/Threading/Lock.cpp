#include "stdafx.h"
#include "Lock.hpp"
#include <windows.h>

#ifdef PROFILE_CRITICAL_SECTIONS
static add_profile_portion_callback add_profile_portion = 0;
void set_add_profile_portion(add_profile_portion_callback callback)
{
    add_profile_portion = callback;
}

struct profiler
{
    u64 m_time;
    LPCSTR m_timer_id;

    IC profiler::profiler(LPCSTR timer_id)
    {
        if (!add_profile_portion)
            return;

        m_timer_id = timer_id;
        m_time = CPU::QPC();
    }

    IC profiler::~profiler()
    {
        if (!add_profile_portion)
            return;

        u64 time = CPU::QPC();
        (*add_profile_portion)(m_timer_id, time - m_time);
    }
};
#endif // PROFILE_CRITICAL_SECTIONS

#ifdef PROFILE_CRITICAL_SECTIONS
Lock::Lock(const char *id) : id(id)
#else
Lock::Lock()
#endif
{ InitializeCriticalSection(&cs); }

Lock::~Lock() { DeleteCriticalSection(&cs); }

#ifdef DEBUG
extern void OutputDebugStackTrace(const char *header);
#endif

void Lock::Enter()
{
#ifdef PROFILE_CRITICAL_SECTIONS
# if 0//def DEBUG
    static bool show_call_stack = false;
    if (show_call_stack)
        OutputDebugStackTrace("----------------------------------------------------");
# endif // DEBUG
    profiler temp(id);
#endif // PROFILE_CRITICAL_SECTIONS
    EnterCriticalSection(&cs);
}

bool Lock::TryEnter() { return !!TryEnterCriticalSection(&cs); }

void Lock::Leave() { LeaveCriticalSection(&cs); }

bool Lock::IsLocked() const
{ return cs.RecursionCount>0 && (DWORD)cs.OwningThread==GetCurrentThreadId(); }
