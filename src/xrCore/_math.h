#pragma once

#include "xr_types.h"

namespace CPU
{
XRCORE_API extern bool HasSSE;
XRCORE_API extern bool HasSSE2;
XRCORE_API extern bool HasSSE42;
XRCORE_API extern bool HasAVX;
XRCORE_API extern bool HasAVX2;
XRCORE_API extern bool HasAVX512F;

XRCORE_API extern u64 qpc_freq;
XRCORE_API extern u32 qpc_counter;

XRCORE_API extern u64 QPC() noexcept;

XRCORE_API u32 GetTicks();
}

extern XRCORE_API void _initialize_cpu();
extern XRCORE_API void _initialize_cpu_thread();
