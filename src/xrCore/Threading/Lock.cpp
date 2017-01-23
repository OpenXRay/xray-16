#include "stdafx.h"
#include "Lock.hpp"

#ifdef CONFIG_PROFILE_LOCKS
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

void Lock::Enter()
{
# if 0//def DEBUG
    static bool show_call_stack = false;
    if (show_call_stack)
        OutputDebugStackTrace("----------------------------------------------------");
# endif // DEBUG
    profiler temp(id);
    mutex.lock();
    isLocked = true;
}
#endif // CONFIG_PROFILE_LOCKS

#ifdef DEBUG
extern void OutputDebugStackTrace(const char *header);
#endif
