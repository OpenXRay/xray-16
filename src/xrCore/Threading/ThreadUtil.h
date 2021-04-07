#pragma once
#if defined(XR_PLATFORM_WINDOWS)
#include <process.h>
#endif

namespace Threading
{
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

XRCORE_API ThreadHandle GetCurrentThreadHandle();

XRCORE_API void SetThreadName(ThreadHandle threadHandle, pcstr name);

XRCORE_API void SetCurrentThreadName(pcstr name);

XRCORE_API bool SpawnThread(EntryFuncType entry, pcstr name, u32 stack, void* arglist);

XRCORE_API void WaitThread(ThreadHandle& threadHandle);

XRCORE_API void CloseThreadHandle(ThreadHandle& threadHandle);

} // namespace Threading
