#include "stdafx.h"
#include "ThreadUtil.h"
#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
#include <pthread.h>
#endif

namespace Threading
{
#if defined(XR_PLATFORM_WINDOWS)
ThreadId GetCurrThreadId() { return GetCurrentThreadId(); }

ThreadHandle GetCurrentThreadHandle() { return GetCurrentThread(); }

void SetThreadName(ThreadHandle threadHandle, pcstr name)
{
    const DWORD MSVC_EXCEPTION = 0x406D1388;
    DWORD threadId = threadHandle != NULL ? GetThreadId(threadHandle) : DWORD(-1);

    struct SThreadNameInfo
    {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    };

    SThreadNameInfo info;
    info.dwType = 0x1000;
    info.szName = name;
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

u32 __stdcall ThreadEntry(void* params)
{
    SThreadStartupInfo* args = (SThreadStartupInfo*)params;
    SetThreadName(NULL, args->threadName);
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
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
ThreadId GetCurrThreadId() { return pthread_self(); }

ThreadHandle GetCurrentThreadHandle() { return pthread_self(); }

void SetThreadName(ThreadHandle threadHandle, pcstr name)
{
    if (!threadHandle)
    {
        pthread_setname_np(pthread_self(), name);
        return;
    }

    pthread_setname_np(threadHandle, name);
}

void* __cdecl ThreadEntry(void* params)
{
    SThreadStartupInfo* args = (SThreadStartupInfo*)params;
    SetThreadName(NULL, args->threadName);
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
#endif
} // namespace Threading
