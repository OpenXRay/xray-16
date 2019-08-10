#include "stdafx.h"
#include "Lock.hpp"
#include <mutex>

FastLock::FastLock() { InitializeSRWLock(&srw); }

void FastLock::Enter() { AcquireSRWLockExclusive(&srw); }

bool FastLock::TryEnter() { return 0 != TryAcquireSRWLockExclusive(&srw); }

void FastLock::Leave() { ReleaseSRWLockExclusive(&srw); }

void FastLock::EnterShared() { AcquireSRWLockShared(&srw); }

bool FastLock::TryEnterShared() { return 0 != TryAcquireSRWLockShared(&srw); }

void FastLock::LeaveShared() { ReleaseSRWLockShared(&srw); }

void* FastLock::GetHandle() { return reinterpret_cast<void*>(&srw); }

////////////////////////////////////////////////////////////////

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

Lock::Lock(const char* id) : lockCounter(0), id(id) { InitializeCriticalSection(&cs); }

void Lock::Enter()
{
    profiler temp(id);
    mutex.lock();
    isLocked = true;
}
#else
Lock::Lock() : lockCounter(0) { InitializeCriticalSection(&cs); }

Lock::~Lock() { DeleteCriticalSection(&cs); }

void Lock::Enter()
{
    EnterCriticalSection(&cs);
    ++lockCounter;
}
#endif // CONFIG_PROFILE_LOCKS

bool Lock::TryEnter()
{
    const bool locked = !!TryEnterCriticalSection(&cs);
    if (locked)
        ++lockCounter;
    return locked;
}

void Lock::Leave()
{
    LeaveCriticalSection(&cs);
    --lockCounter;
}

void* Lock::GetHandle() { return reinterpret_cast<void*>(&cs); }
