#pragma once
#include "xrCore/xrCore.h"

#include <mutex>
#include <atomic>

#ifdef CONFIG_PROFILE_LOCKS
typedef void(*add_profile_portion_callback) (LPCSTR id, const u64& time);
void XRCORE_API set_add_profile_portion(add_profile_portion_callback callback);

# define STRINGIZER_HELPER(a) #a
# define STRINGIZER(a) STRINGIZER_HELPER(a)
# define CONCATENIZE_HELPER(a,b) a##b
# define CONCATENIZE(a,b) CONCATENIZE_HELPER(a,b)
# define MUTEX_PROFILE_PREFIX_ID #mutexes/
# define MUTEX_PROFILE_ID(a) STRINGIZER(CONCATENIZE(MUTEX_PROFILE_PREFIX_ID,a))
#endif // CONFIG_PROFILE_LOCKS

class XRCORE_API Lock
{
public:
#ifdef CONFIG_PROFILE_LOCKS
    Lock(const char *id) : isLocked(false), id(id) { }
#else
    Lock() : isLocked(false) { }
#endif

    Lock(const Lock &) = delete;
    Lock operator=(const Lock &) = delete;

#ifdef CONFIG_PROFILE_LOCKS
    void Enter();
#else
    void Enter() { return mutex.lock(); isLocked = true; }
#endif

    bool TryEnter()
    {
        bool locked = mutex.try_lock();
        if (locked)
        {
            isLocked = true;
        }
        return locked;
    }

    void Leave() { return mutex.unlock(); isLocked = false; }

    bool IsLocked() const { return isLocked; }

private:
    std::mutex mutex;
    std::atomic_bool isLocked;
#ifdef CONFIG_PROFILE_LOCKS
    const char *id;
#endif
};
