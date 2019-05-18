#pragma once

#include "_types.h"
#include "cpuid.h"

namespace FPU
{
XRCORE_API void m24();
XRCORE_API void m24r();
XRCORE_API void m53();
XRCORE_API void m53r();
XRCORE_API void m64();
XRCORE_API void m64r();
}

namespace CPU
{
XRCORE_API extern u64 qpc_freq;
XRCORE_API extern u32 qpc_counter;

XRCORE_API extern processor_info ID;
XRCORE_API extern u64 QPC() noexcept;

XRCORE_API u64 GetCLK();
XRCORE_API u32 GetCurrentCPU();
}

extern XRCORE_API void _initialize_cpu();
extern XRCORE_API void _initialize_cpu_thread();

// threading
using thread_t = void(void*);
extern XRCORE_API void thread_name(const char* name);
extern XRCORE_API void thread_spawn(thread_t* entry, const char* name, unsigned stack, void* arglist);

#if defined(LINUX)
XRCORE_API DWORD timeGetTime();
#endif
