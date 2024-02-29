#pragma once
#if defined(XR_PLATFORM_WINDOWS)
#include <process.h>
#endif

namespace Threading
{
enum class priority_class
{
    idle,
    below_normal,
    normal,
    above_normal,
    high,
    realtime,
};

enum class priority_level
{
    idle,
    lowest,
    below_normal,
    normal,
    above_normal,
    highest,
    time_critical,
};

#ifdef XR_PLATFORM_WINDOWS
using ThreadHandle = HANDLE;
using ThreadId = u32;
#else
using ThreadHandle = pthread_t;
using ThreadId = pthread_t;
#endif
using EntryFuncType = void (*)(void*);

struct SThreadStartupInfo
{
    pcstr threadName;
    EntryFuncType entryFunc;
    void* argList;
};

//////////////////////////////////////////////////////////////

XRCORE_API ThreadId GetCurrThreadId();

XRCORE_API bool ThreadIdsAreEqual(ThreadId left, ThreadId right);

XRCORE_API void SetCurrentThreadName(pcstr name);

XRCORE_API priority_level GetCurrentThreadPriorityLevel();
XRCORE_API priority_class GetCurrentProcessPriorityClass();

XRCORE_API void SetCurrentThreadPriorityLevel(priority_level prio);
XRCORE_API void SetCurrentProcessPriorityClass(priority_class cls);

XRCORE_API bool SpawnThread(EntryFuncType entry, pcstr name, u32 stack, void* arglist);

XRCORE_API void WaitThread(ThreadHandle& threadHandle);

XRCORE_API void CloseThreadHandle(ThreadHandle& threadHandle);

} // namespace Threading
