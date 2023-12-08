#pragma once

#include "xr_types.h"
#include <thread>

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
XRCORE_API extern bool HasSSE;

XRCORE_API extern u64 qpc_freq;
XRCORE_API extern u32 qpc_counter;

XRCORE_API extern u64 QPC() noexcept;

XRCORE_API u32 GetTicks();

// GermanAizek: This implementation supports both conventional single-cpu PC configurations
//              and multi-cpu system on NUMA (Non-uniform_memory_access) architecture
XRCORE_API size_t GetThreadsCounts() noexcept;
XRCORE_API void setThreadAffinityAllGroupCores(HANDLE handle) noexcept;
}

extern XRCORE_API void _initialize_cpu();
extern XRCORE_API void _initialize_cpu_thread();
