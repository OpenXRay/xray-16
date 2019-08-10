#pragma once
#include <atomic>
#include "Common/Noncopyable.hpp"

#ifdef CONFIG_PROFILE_LOCKS
typedef void (*add_profile_portion_callback)(LPCSTR id, const u64& time);
void XRCORE_API set_add_profile_portion(add_profile_portion_callback callback);

#define STRINGIZER_HELPER(a) #a
#define STRINGIZER(a) STRINGIZER_HELPER(a)
#define CONCATENIZE_HELPER(a, b) a##b
#define CONCATENIZE(a, b) CONCATENIZE_HELPER(a, b)
#define MUTEX_PROFILE_PREFIX_ID #mutexes /
#define MUTEX_PROFILE_ID(a) STRINGIZER(CONCATENIZE(MUTEX_PROFILE_PREFIX_ID, a))
#endif // CONFIG_PROFILE_LOCKS

// Non recursive
class XRCORE_API FastLock : Noncopyable
{
public:
    enum EFastLockType : ULONG
    {
        Exclusive = 0,
        Shared = CONDITION_VARIABLE_LOCKMODE_SHARED
    };

public:
    FastLock();
    ~FastLock(){};

    void Enter();
    bool TryEnter();
    void Leave();

    void EnterShared();
    bool TryEnterShared();
    void LeaveShared();

    void* GetHandle();

private:
    SRWLOCK srw;
};

/////////////////////////////////////////////////////

class XRCORE_API Lock : Noncopyable
{
    CRITICAL_SECTION cs;

public:
#ifdef CONFIG_PROFILE_LOCKS
    Lock(const char* id);
#else
    Lock();
#endif
    ~Lock();

    void Enter();
    bool TryEnter();
    void Leave();

    bool IsLocked() const { return !!lockCounter; }

    void* GetHandle();

private:
    std::atomic_int lockCounter;
#ifdef CONFIG_PROFILE_LOCKS
    const char* id;
#endif
};
