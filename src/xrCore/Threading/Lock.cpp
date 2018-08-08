#include "stdafx.h"
#include "Lock.hpp"
#include <mutex>

struct LockImpl
{
#ifdef WINDOWS
    CRITICAL_SECTION cs;

    LockImpl() { InitializeCriticalSection(&cs); }
    ~LockImpl() { DeleteCriticalSection(&cs); }

    ICF void Lock() { EnterCriticalSection(&cs); }
    ICF void Unlock() { LeaveCriticalSection(&cs); }
    ICF bool TryLock() { return !!TryEnterCriticalSection(&cs); }
#else
    std::recursive_mutex mutex;

    ICF void Lock() { mutex.lock(); }
    ICF void Unlock() { mutex.unlock(); }
    ICF bool TryLock() { return mutex.try_lock(); }
#endif
};

#ifdef CONFIG_PROFILE_LOCKS
static add_profile_portion_callback add_profile_portion = 0;
void set_add_profile_portion(add_profile_portion_callback callback) { add_profile_portion = callback; }
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

Lock::Lock(const char* id) : impl(new LockImpl), lockCounter(0), id(id) {}

void Lock::Enter()
{
#if 0 // def DEBUG
    static bool show_call_stack = false;
    if (show_call_stack)
        OutputDebugStackTrace("----------------------------------------------------");
#endif // DEBUG
    profiler temp(id);
    mutex.lock();
    isLocked = true;
}
#else
Lock::Lock() : impl(new LockImpl), lockCounter(0) {}

Lock::~Lock() { delete impl; }

void Lock::Enter()
{
    impl->Lock();
    lockCounter++;
}
#endif // CONFIG_PROFILE_LOCKS

bool Lock::TryEnter()
{
    const bool locked = impl->TryLock();
    if (locked)
        ++lockCounter;
    return locked;
}

void Lock::Leave()
{
    impl->Unlock();
    --lockCounter;
}

#ifdef DEBUG
extern void OutputDebugStackTrace(const char* header);
#endif
