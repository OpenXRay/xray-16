#pragma once
#include <atomic>

#include "Common/Noncopyable.hpp"

#ifdef CONFIG_PROFILE_LOCKS
typedef void (*add_profile_portion_callback)(LPCSTR id, const u64& time);
void XRCORE_API set_add_profile_portion(add_profile_portion_callback callback);

#define MUTEX_PROFILE_PREFIX_ID #mutexes /
#define MUTEX_PROFILE_ID(a) MACRO_TO_STRING(CONCATENIZE(MUTEX_PROFILE_PREFIX_ID, a))
#endif // CONFIG_PROFILE_LOCKS

class XRCORE_API Lock : Noncopyable
{
    struct LockImpl* impl;
public:
#ifdef CONFIG_PROFILE_LOCKS
    Lock(const char* id);
#else
    Lock();
#endif
    ~Lock();

#ifdef CONFIG_PROFILE_LOCKS
    void Enter();
#else
    void Enter();
#endif

    bool TryEnter();

    void Leave();

    bool IsLocked() const { return !!lockCounter; }

private:
    std::atomic_int lockCounter;
#ifdef CONFIG_PROFILE_LOCKS
    const char* id;
#endif
};
