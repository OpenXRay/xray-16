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
    return pthread_setname_np(threadId, "%s", name);
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
ThreadId GetCurrThreadId() { return GetCurrentThreadId(); }

bool ThreadIdsAreEqual(ThreadId left, ThreadId right) { return left == right; }

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

void SetCurrentThreadName(pcstr name)
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
    default: [[fallthrough]]
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
   default: [[fallthrough]]
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
    default: [[fallthrough]]
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
    default: [[fallthrough]]
    case priority_class::normal:       dwPriorityClass = NORMAL_PRIORITY_CLASS; break;
    case priority_class::above_normal: dwPriorityClass = ABOVE_NORMAL_PRIORITY_CLASS; break;
    case priority_class::high:         dwPriorityClass = HIGH_PRIORITY_CLASS; break;
    case priority_class::realtime:     dwPriorityClass = REALTIME_PRIORITY_CLASS; break;
    }
    SetPriorityClass(GetCurrentProcess(), dwPriorityClass);
}

u32 __stdcall ThreadEntry(void* params)
{
    SThreadStartupInfo* args = (SThreadStartupInfo*)params;
    SetCurrentThreadName(args->threadName);
    EntryFuncType entry = args->entryFunc;
    void* arglist = args->argList;
    xr_delete(args);
    _initialize_cpu_thread();

    // call
    entry(arglist);

    return 0;
}

bool SpawnThread(EntryFuncType entry, pcstr name, u32 stack, void* arglist)
{
    xrDebug::Initialize(Core.Params);

    SThreadStartupInfo* info = xr_new<SThreadStartupInfo>();
    info->threadName = name;
    info->entryFunc = entry;
    info->argList = arglist;
    ThreadHandle threadHandle = (ThreadHandle)_beginthreadex(NULL, stack, ThreadEntry, info, CREATE_SUSPENDED, NULL);

    if (!threadHandle)
    {
        xr_string errMsg = xrDebug::ErrorToString(GetLastError());
        Msg("SpawnThread: can't create thread '%s'. Error Msg: '%s'", name, errMsg.c_str());
        return false;
    }

    ResumeThread(threadHandle);
    return true;
}

void WaitThread(ThreadHandle& threadHandle) { WaitForSingleObject(threadHandle, INFINITE); }

void CloseThreadHandle(ThreadHandle& threadHandle)
{
    if (threadHandle)
    {
        CloseHandle(threadHandle);
        threadHandle = nullptr;
    }
}
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
ThreadId GetCurrThreadId() { return pthread_self(); }

bool ThreadIdsAreEqual(ThreadId left, ThreadId right) { return !!pthread_equal(left, right); }

void SetCurrentThreadName(pcstr name)
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

void* __cdecl ThreadEntry(void* params)
{
    SThreadStartupInfo* args = (SThreadStartupInfo*)params;
    SetCurrentThreadName(args->threadName);
    EntryFuncType entry = args->entryFunc;
    void* arglist = args->argList;
    xr_delete(args);
    _initialize_cpu_thread();

    // call
    entry(arglist);

    return nullptr;
}

bool SpawnThread(EntryFuncType entry, pcstr name, u32 stack, void* arglist)
{
    xrDebug::Initialize(Core.Params);

    SThreadStartupInfo* info = xr_new<SThreadStartupInfo>();
    info->threadName = name;
    info->entryFunc = entry;
    info->argList = arglist;

    pthread_t handle = 0;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack);

    int res = pthread_create(&handle, &attr, &ThreadEntry, info);
    pthread_attr_destroy(&attr);

    if (res != 0)
    {
        Msg("SpawnThread: can't create thread '%s'.", name);
        return false;
    }

    return true;
}

void WaitThread(ThreadHandle& threadHandle) { pthread_join(threadHandle, NULL); }

void CloseThreadHandle(ThreadHandle& threadHandle) { pthread_detach(threadHandle); }
#else
#   error Add threading code for your platform
#endif
} // namespace Threading
