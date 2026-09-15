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

namespace xray::profiler
{

struct ZoneFunctionName
{
    static constexpr size_t Capacity = 96;
    char data[Capacity] = {};
};

constexpr bool ZoneIsIdentChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

constexpr bool ZoneStartsWith(const char* text, const char* prefix)
{
    for (; *prefix; ++text, ++prefix)
        if (*text != *prefix)
            return false;
    return true;
}

constexpr ZoneFunctionName QualifyZoneFunction(const char* pretty)
{
    size_t length = 0;
    while (pretty[length])
        ++length;

    size_t end = length;
    for (size_t i = 0; i < length; ++i)
    {
        if (pretty[i] != '(')
            continue;
        if (ZoneStartsWith(pretty + i + 1, "anonymous") || (i >= 8 && ZoneStartsWith(pretty + i - 8, "operator")))
        {
            while (i < length && pretty[i] != ')')
                ++i;
            continue;
        }
        end = i;
        break;
    }

    size_t start = end;
    int depth = 0;
    while (start > 0)
    {
        const char c = pretty[start - 1];
        if (c == '>')
        {
            ++depth;
            --start;
            continue;
        }
        if (c == '<')
        {
            if (depth > 0)
                --depth;
            --start;
            continue;
        }
        if (depth > 0 || ZoneIsIdentChar(c) || c == ':' || c == '~')
        {
            --start;
            continue;
        }
        if (c == ')' && start >= 10 && ZoneStartsWith(pretty + start - 10, "operator()"))
        {
            start -= 10;
            continue;
        }
        break;
    }

    size_t keep = start;
    size_t separators = 0;
    depth = 0;
    for (size_t i = end; i > start + 1; --i)
    {
        const char c = pretty[i - 1];
        if (c == '>')
            ++depth;
        else if (c == '<')
            --depth;
        else if (depth == 0 && c == ':' && pretty[i - 2] == ':')
        {
            if (++separators == 2)
            {
                keep = i;
                break;
            }
            --i;
        }
    }

    while (keep < end && pretty[keep] == ':')
        ++keep;
    ZoneFunctionName result{};
    size_t written = 0;
    for (size_t i = keep; i < end && written < ZoneFunctionName::Capacity - 1; ++i)
        result.data[written++] = pretty[i];
    if (written == 0)
        for (size_t i = 0; i < length && written < ZoneFunctionName::Capacity - 1; ++i)
            result.data[written++] = pretty[i];
    result.data[written] = '\0';
    return result;
}

}

#if defined(__clang__) || defined(__GNUC__)
#define XRAY_ZONE_FUNCTION_DECL(var) \
    static constexpr ::xray::profiler::ZoneFunctionName var = ::xray::profiler::QualifyZoneFunction(__PRETTY_FUNCTION__);
#define XRAY_ZONE_FUNCTION_NAME(var) var.data
#else
#define XRAY_ZONE_FUNCTION_DECL(var)
#define XRAY_ZONE_FUNCTION_NAME(var) __FUNCTION__
#endif

#define XRAY_ZONE_SCOPED_IMPL \
    XRAY_ZONE_FUNCTION_DECL(PROFILER_UNIQUE_VAR(__xr_zone_name_)) \
    static ::xray::profiler::ZoneInfo PROFILER_UNIQUE_VAR(__xr_zone_info_) \
        {XRAY_ZONE_FUNCTION_NAME(PROFILER_UNIQUE_VAR(__xr_zone_name_)), __FILE__, static_cast<u32>(__LINE__), ::xray::profiler::INVALID_ZONE_ID}; \
    ::xray::profiler::CPUZoneScope PROFILER_UNIQUE_VAR(__xr_zone_scope_) \
        {&PROFILER_UNIQUE_VAR(__xr_zone_info_)}

#define XRAY_ZONE_SCOPED_N_IMPL(name) \
    static ::xray::profiler::ZoneInfo PROFILER_UNIQUE_VAR(__xr_zone_info_) \
        {name, __FILE__, static_cast<u32>(__LINE__), ::xray::profiler::INVALID_ZONE_ID}; \
    ::xray::profiler::CPUZoneScope PROFILER_UNIQUE_VAR(__xr_zone_scope_) \
        {&PROFILER_UNIQUE_VAR(__xr_zone_info_)}

#define XRAY_ZONE_NAMED_IMPL(varname, active) \
    XRAY_ZONE_FUNCTION_DECL(PROFILER_CONCAT(__xr_zone_name_, varname)) \
    static ::xray::profiler::ZoneInfo PROFILER_CONCAT(__xr_zone_info_, varname) \
        {XRAY_ZONE_FUNCTION_NAME(PROFILER_CONCAT(__xr_zone_name_, varname)), __FILE__, static_cast<u32>(__LINE__), ::xray::profiler::INVALID_ZONE_ID}; \
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
