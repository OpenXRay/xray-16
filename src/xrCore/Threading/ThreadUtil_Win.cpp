#include "stdafx.h"
#include "ThreadUtil.h"

namespace ThreadUtil
{
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

bool CreateThread(EntryFuncType entry, pcstr name, u32 stack, void* arglist)
{
    xrDebug::Initialize(Core.Params);

    SThreadStartupInfo* info = new SThreadStartupInfo();
    info->threadName = name;
    info->entryFunc = entry;
    info->argList = arglist;
    ThreadHandle threadHandle = (ThreadHandle)_beginthreadex(NULL, stack, ThreadEntry, info, CREATE_SUSPENDED, NULL);

    if (!threadHandle)
    {
        xr_string errMsg = xrDebug::ErrorToString(GetLastError());
        Msg("CreateThread: can't create thread '%s'. Error Msg: '%s'", name, errMsg.c_str());
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
} // namespace ThreadUtil
