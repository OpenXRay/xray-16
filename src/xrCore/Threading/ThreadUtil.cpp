#include "stdafx.h"
#include "ThreadUtil.h"

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
#   include <pthread.h>

#   if defined(XR_PLATFORM_OPENBSD) || (defined(XR_PLATFORM_FREEBSD) && __FreeBSD_version < 1201519)
#   include <pthread_np.h>

static int pthread_setname_np(pthread_t threadId, const char* name)
{
    pthread_set_name_np(threadId, name);
    return 0;
}
#   elif defined(XR_PLATFORM_NETBSD)
static int pthread_setname_np(pthread_t threadId, const char* name)
{
    return pthread_setname_np(threadId, "%s", const_cast<char*>(name));
}
#   elif defined(XR_PLATFORM_APPLE)
static int pthread_setname_np(pthread_t /*threadId*/, const char* name)
{
    return pthread_setname_np(name);
}
#   endif
#endif

namespace Threading
{
#if defined(XR_PLATFORM_WINDOWS)
void SetThreadNameImpl(DWORD threadId, pcstr name)
{
    const DWORD MSVC_EXCEPTION = 0x406D1388;

    struct SThreadNameInfo
    {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    };

    constexpr char namePrefix[] = "X-Ray ";
    constexpr auto namePrefixSize = std::size(namePrefix); // includes null-character intentionally
    auto fullNameSize = xr_strlen(name) + namePrefixSize;
    auto fullName = static_cast<pstr>(xr_alloca(fullNameSize));
    strconcat(fullNameSize, fullName, namePrefix, name);

    SThreadNameInfo info;
    info.dwType = 0x1000;
    info.szName = fullName;
    info.dwThreadID = threadId;
    info.dwFlags = 0;

    __try
    {
        RaiseException(MSVC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}

void SetCurrentThreadName(cpcstr name)
{
    SetThreadNameImpl(-1, name);
}

priority_level GetCurrentThreadPriorityLevel()
{
    switch (GetThreadPriority(GetCurrentThread()))
    {
    case THREAD_PRIORITY_IDLE:          return priority_level::idle;
    case THREAD_PRIORITY_LOWEST:        return priority_level::lowest;
    case THREAD_PRIORITY_BELOW_NORMAL:  return priority_level::below_normal;
    default: [[fallthrough]];
    case THREAD_PRIORITY_NORMAL:        return priority_level::normal;
    case THREAD_PRIORITY_ABOVE_NORMAL:  return priority_level::above_normal;
    case THREAD_PRIORITY_HIGHEST:       return priority_level::highest;
    case THREAD_PRIORITY_TIME_CRITICAL: return priority_level::time_critical;
    }
}

priority_class GetCurrentProcessPriorityClass()
{
   switch (GetPriorityClass(GetCurrentProcess()))
   {
   case IDLE_PRIORITY_CLASS:         return priority_class::idle;
   case BELOW_NORMAL_PRIORITY_CLASS: return priority_class::below_normal;
   default: [[fallthrough]];
   case NORMAL_PRIORITY_CLASS:       return priority_class::normal;
   case ABOVE_NORMAL_PRIORITY_CLASS: return priority_class::above_normal;
   case HIGH_PRIORITY_CLASS:         return priority_class::high;
   case REALTIME_PRIORITY_CLASS:     return priority_class::realtime;
   }
}

void SetCurrentThreadPriorityLevel(priority_level prio)
{
    int nPriority;
    switch (prio)
    {
    case priority_level::idle:          nPriority = THREAD_PRIORITY_IDLE; break;
    case priority_level::lowest:        nPriority = THREAD_PRIORITY_LOWEST; break;
    case priority_level::below_normal:  nPriority = THREAD_PRIORITY_BELOW_NORMAL; break;
    default: [[fallthrough]];
    case priority_level::normal:        nPriority = THREAD_PRIORITY_NORMAL; break;
    case priority_level::above_normal:  nPriority = THREAD_PRIORITY_ABOVE_NORMAL; break;
    case priority_level::highest:       nPriority = THREAD_PRIORITY_HIGHEST; break;
    case priority_level::time_critical: nPriority = THREAD_PRIORITY_TIME_CRITICAL; break;
    }
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
}

void SetCurrentProcessPriorityClass(priority_class cls)
{
    DWORD dwPriorityClass;
    switch (cls)
    {
    case priority_class::idle:         dwPriorityClass = IDLE_PRIORITY_CLASS; break;
    case priority_class::below_normal: dwPriorityClass = BELOW_NORMAL_PRIORITY_CLASS; break;
    default: [[fallthrough]];
    case priority_class::normal:       dwPriorityClass = NORMAL_PRIORITY_CLASS; break;
    case priority_class::above_normal: dwPriorityClass = ABOVE_NORMAL_PRIORITY_CLASS; break;
    case priority_class::high:         dwPriorityClass = HIGH_PRIORITY_CLASS; break;
    case priority_class::realtime:     dwPriorityClass = REALTIME_PRIORITY_CLASS; break;
    }
    SetPriorityClass(GetCurrentProcess(), dwPriorityClass);
}
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
void SetCurrentThreadName(cpcstr name)
{
    if (auto error = pthread_setname_np(pthread_self(), name) != 0)
    {
        Msg("SetCurrentThreadName: failed to set thread name to '%s'. Errno: '%d'", name, error);
    }
}

priority_level GetCurrentThreadPriorityLevel()
{
    return priority_level::normal;
}

priority_class GetCurrentProcessPriorityClass()
{
    return priority_class::normal;
}

void SetCurrentThreadPriorityLevel(priority_level prio)
{

}

void SetCurrentProcessPriorityClass(priority_class cls)
{

}
#else
void SetCurrentThreadName(cpcstr name) {}

priority_level GetCurrentThreadPriorityLevel() { return priority_level::normal; }
priority_class GetCurrentProcessPriorityClass() { return priority_class::normal; }

void SetCurrentThreadPriorityLevel(priority_level prio) {}
void SetCurrentProcessPriorityClass(priority_class cls) {}
#endif
} // namespace Threading
