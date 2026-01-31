#pragma once

#include "ProfilerTypes.h"
#include "CPUProfiler.h"

#ifdef TRACY_ENABLE
    #include <tracy/Tracy.hpp>
    #define XRAY_TRACY_ENABLED 1
#else
    #define XRAY_TRACY_ENABLED 0
#endif

namespace xray::profiler
{

XRCORE_API void Initialize();
XRCORE_API void Shutdown();

XRCORE_API void FrameStart();
XRCORE_API void FrameEnd();

XRCORE_API CPUProfiler& GetCPUProfiler();

XRCORE_API bool IsEnabled();
XRCORE_API void SetEnabled(bool enabled);

}

#define PROFILER_CONCAT_IMPL(a, b) a##b
#define PROFILER_CONCAT(a, b) PROFILER_CONCAT_IMPL(a, b)
#define PROFILER_UNIQUE_VAR(prefix) PROFILER_CONCAT(prefix, __LINE__)

#define XRAY_ZONE_SCOPED_IMPL \
    static ::xray::profiler::ZoneInfo PROFILER_UNIQUE_VAR(__xr_zone_info_) \
        {__FUNCTION__, __FILE__, static_cast<u32>(__LINE__), ::xray::profiler::INVALID_ZONE_ID}; \
    ::xray::profiler::CPUZoneScope PROFILER_UNIQUE_VAR(__xr_zone_scope_) \
        {&PROFILER_UNIQUE_VAR(__xr_zone_info_)}

#define XRAY_ZONE_SCOPED_N_IMPL(name) \
    static ::xray::profiler::ZoneInfo PROFILER_UNIQUE_VAR(__xr_zone_info_) \
        {name, __FILE__, static_cast<u32>(__LINE__), ::xray::profiler::INVALID_ZONE_ID}; \
    ::xray::profiler::CPUZoneScope PROFILER_UNIQUE_VAR(__xr_zone_scope_) \
        {&PROFILER_UNIQUE_VAR(__xr_zone_info_)}

#define XRAY_ZONE_NAMED_IMPL(varname, active) \
    static ::xray::profiler::ZoneInfo PROFILER_CONCAT(__xr_zone_info_, varname) \
        {__FUNCTION__, __FILE__, static_cast<u32>(__LINE__), ::xray::profiler::INVALID_ZONE_ID}; \
    ::xray::profiler::CPUZoneScope PROFILER_CONCAT(__xr_zone_, varname) \
        {(active) ? &PROFILER_CONCAT(__xr_zone_info_, varname) : nullptr}

#define XRAY_ZONE_NAMED_N_IMPL(varname, name, active) \
    static ::xray::profiler::ZoneInfo PROFILER_CONCAT(__xr_zone_info_, varname) \
        {name, __FILE__, static_cast<u32>(__LINE__), ::xray::profiler::INVALID_ZONE_ID}; \
    ::xray::profiler::CPUZoneScope PROFILER_CONCAT(__xr_zone_, varname) \
        {(active) ? &PROFILER_CONCAT(__xr_zone_info_, varname) : nullptr}

#if XRAY_TRACY_ENABLED

#undef ZoneScoped
#undef ZoneScopedN
#undef ZoneScopedC
#undef ZoneScopedNC
#undef ZoneScopedS
#undef ZoneScopedNS
#undef ZoneScopedCS
#undef ZoneScopedNCS

#define ZoneScoped \
    ZoneNamed(___tracy_scoped_zone, true); \
    XRAY_ZONE_SCOPED_IMPL

#define ZoneScopedN(name) \
    ZoneNamedN(___tracy_scoped_zone, name, true); \
    XRAY_ZONE_SCOPED_N_IMPL(name)

#define ZoneScopedC(color) \
    ZoneNamedC(___tracy_scoped_zone, color, true); \
    XRAY_ZONE_SCOPED_IMPL

#define ZoneScopedNC(name, color) \
    ZoneNamedNC(___tracy_scoped_zone, name, color, true); \
    XRAY_ZONE_SCOPED_N_IMPL(name)

#define ZoneScopedS(depth) \
    ZoneNamedS(___tracy_scoped_zone, depth, true); \
    XRAY_ZONE_SCOPED_IMPL

#define ZoneScopedNS(name, depth) \
    ZoneNamedNS(___tracy_scoped_zone, name, depth, true); \
    XRAY_ZONE_SCOPED_N_IMPL(name)

#define ZoneScopedCS(color, depth) \
    ZoneNamedCS(___tracy_scoped_zone, color, depth, true); \
    XRAY_ZONE_SCOPED_IMPL

#define ZoneScopedNCS(name, color, depth) \
    ZoneNamedNCS(___tracy_scoped_zone, name, color, depth, true); \
    XRAY_ZONE_SCOPED_N_IMPL(name)

#else

#define ZoneScoped XRAY_ZONE_SCOPED_IMPL
#define ZoneScopedN(name) XRAY_ZONE_SCOPED_N_IMPL(name)
#define ZoneScopedC(color) ZoneScoped
#define ZoneScopedNC(name, color) ZoneScopedN(name)

#define ZoneText(text, size) (void)0
#define ZoneTextV(varname, text, size) (void)0
#define ZoneName(name, size) (void)0
#define ZoneNameV(varname, name, size) (void)0
#define ZoneValue(value) (void)0

#define FrameMark (void)0
#define FrameMarkNamed(name) (void)0
#define FrameMarkStart(name) (void)0
#define FrameMarkEnd(name) (void)0

#define TracyAlloc(ptr, size) (void)0
#define TracyFree(ptr) (void)0
#define TracyAllocN(ptr, size, name) (void)0
#define TracyFreeN(ptr, name) (void)0
#define TracySecureAlloc(ptr, size) (void)0
#define TracySecureFree(ptr) (void)0
#define TracySecureAllocN(ptr, size, name) (void)0
#define TracySecureFreeN(ptr, name) (void)0

#define TracyMessage(text, size) (void)0
#define TracyMessageL(text) (void)0
#define TracyMessageC(text, size, color) (void)0
#define TracyMessageLC(text, color) (void)0
#define TracyAppInfo(text, size) (void)0
#define TracyPlot(name, value) (void)0
#define TracyPlotConfig(name, type, step, fill, color) (void)0

#define TracyLockable(type, varname) type varname
#define TracyLockableN(type, varname, desc) type varname
#define TracySharedLockable(type, varname) type varname
#define TracySharedLockableN(type, varname, desc) type varname
#define LockableBase(type) type
#define SharedLockableBase(type) type
#define LockMark(varname) (void)0
#define LockableName(varname, name, size) (void)0

#define ZoneScopedS(depth) ZoneScoped
#define ZoneScopedNS(name, depth) ZoneScopedN(name)
#define ZoneScopedCS(color, depth) ZoneScoped
#define ZoneScopedNCS(name, color, depth) ZoneScopedN(name)

#define ZoneNamed(varname, active) XRAY_ZONE_NAMED_IMPL(varname, active)
#define ZoneNamedN(varname, name, active) XRAY_ZONE_NAMED_N_IMPL(varname, name, active)
#define ZoneNamedC(varname, color, active) ZoneNamed(varname, active)
#define ZoneNamedNC(varname, name, color, active) ZoneNamedN(varname, name, active)
#define ZoneNamedS(varname, depth, active) ZoneNamed(varname, active)
#define ZoneNamedNS(varname, name, depth, active) ZoneNamedN(varname, name, active)
#define ZoneNamedCS(varname, color, depth, active) ZoneNamed(varname, active)
#define ZoneNamedNCS(varname, name, color, depth, active) ZoneNamedN(varname, name, active)

#define ZoneTransient(varname, active) ZoneNamed(varname, active)
#define ZoneTransientN(varname, name, active) ZoneNamedN(varname, name, active)

#endif

using TracyD3D11Ctx = void*;
#define TracyD3D11Context(device, context) nullptr
#define TracyD3D11Destroy(ctx) (void)0
#define TracyD3D11ContextName(ctx, name, size) (void)0
#define TracyD3D11NewFrame(ctx) (void)0
#define TracyD3D11Zone(ctx, name) (void)0
#define TracyD3D11ZoneC(ctx, name, color) (void)0
#define TracyD3D11Collect(ctx) (void)0

using TracyD3D12Ctx = void*;
#define TracyD3D12Context(device, queue) nullptr
#define TracyD3D12Destroy(ctx) (void)0
#define TracyD3D12ContextName(ctx, name, size) (void)0
#define TracyD3D12NewFrame(ctx) (void)0
#define TracyD3D12Zone(ctx, cmdList, name) (void)0
#define TracyD3D12ZoneC(ctx, cmdList, name, color) (void)0
#define TracyD3D12Collect(ctx) (void)0
