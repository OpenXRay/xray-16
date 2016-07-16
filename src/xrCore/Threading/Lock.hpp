#pragma once
#include "xrCore/xrCore_impexp.h"

#include <mutex>
#include <atomic>

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

class XRCORE_API Lock
{
public:
#ifdef CONFIG_PROFILE_LOCKS
    Lock(const char* id) : lockCounter(0), id(id) {}
#else
    Lock() : lockCounter(0) {}
#endif

    Lock(const Lock&) = delete;
    Lock operator=(const Lock&) = delete;

#ifdef CONFIG_PROFILE_LOCKS
    void Enter();
#else
    void Enter()
    {
        mutex.lock();
        lockCounter++;
    }
#endif

    bool TryEnter()
    {
        bool locked = mutex.try_lock();
        if (locked)
            lockCounter++;
        return locked;
    }

    void Leave()
    {
        mutex.unlock();
        lockCounter--;
    }

    bool IsLocked() const { return !!lockCounter; }
private:
    std::recursive_mutex mutex;
    std::atomic_int lockCounter;
#ifdef CONFIG_PROFILE_LOCKS
    const char* id;
#endif
};
