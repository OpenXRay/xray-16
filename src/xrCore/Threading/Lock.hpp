#pragma once
#include "xrCore/xrCore.h"

#ifdef PROFILE_CRITICAL_SECTIONS
typedef void(*add_profile_portion_callback) (LPCSTR id, const u64& time);
void XRCORE_API set_add_profile_portion(add_profile_portion_callback callback);

# define STRINGIZER_HELPER(a) #a
# define STRINGIZER(a) STRINGIZER_HELPER(a)
# define CONCATENIZE_HELPER(a,b) a##b
# define CONCATENIZE(a,b) CONCATENIZE_HELPER(a,b)
# define MUTEX_PROFILE_PREFIX_ID #mutexes/
# define MUTEX_PROFILE_ID(a) STRINGIZER(CONCATENIZE(MUTEX_PROFILE_PREFIX_ID,a))
#endif // PROFILE_CRITICAL_SECTIONS

class XRCORE_API Lock
{
private:
    friend class AutoLock;
    CRITICAL_SECTION cs;
#ifdef PROFILE_CRITICAL_SECTIONS
    const char *id;
#endif

public:
#ifdef PROFILE_CRITICAL_SECTIONS
    Lock(const char *id);
#else
    Lock();
#endif
    ~Lock();

    Lock(const Lock &) = delete;
    Lock operator=(const Lock &) = delete;

    void Enter();
    bool TryEnter();
    void Leave();
    bool IsLocked() const;
};
