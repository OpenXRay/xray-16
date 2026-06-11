#pragma once

#include "xr_types.h"

#include <cstddef>

// Per-frame allocation diagnostics.
//
// Counting happens at the xrMemory choke point: the global operator new/delete
// overrides route here, as do ImGui, Lua/luabind and the xr_* helpers. SDL,
// OpenAL and driver-internal allocations bypass xrMemory and are not seen.
// Byte counts use the allocator's usable size where available (mimalloc,
// Darwin/glibc malloc), so alloc and free bytes net out exactly.
//
// Attribution (which code allocated) lives in the CPU profiler: zones snapshot
// the per-thread counters below and record deltas per zone. This module owns
// only the counters, the per-frame report, a generic keyed breakdown table and
// the DisallowHeapAlloc enforcement scope.
namespace xray::memstats
{

// ------------------------------ core counters ------------------------------

// Called by xrMemory on every allocation/free.
XRCORE_API void CountAlloc(size_t size);
XRCORE_API void CountFree(size_t size);

XRCORE_API u64 AllocCallsTotal();
XRCORE_API u64 AllocBytesTotal();
XRCORE_API u64 AllocCallsThread();
XRCORE_API u64 AllocBytesThread();
XRCORE_API u64 FreeCallsThread();
XRCORE_API u64 FreeBytesThread();

// ------------------------- keyed breakdown tables --------------------------
// Generic "group allocations by interned key" channel for breakdowns that are
// not scopes (e.g. per-object-class UpdateCL costs). Keys are matched by
// POINTER, so they must be literals or interned strings (shared_str/pass
// names) that outlive the frame.

enum class Table : int
{
    ObjectClass,
    Count
};

constexpr int tableCount = static_cast<int>(Table::Count);
constexpr int namedCapacity = 512;

struct NamedEntry
{
    const char* key = nullptr;
    u64 calls = 0;
    u64 bytes = 0;
    u64 frees = 0;
    u64 freeBytes = 0;
};

XRCORE_API void AddNamed(Table table, const char* key, u64 calls, u64 bytes, u64 frees = 0, u64 freeBytes = 0);

XRCORE_API void SetObjectClassProfiling(bool enabled);
XRCORE_API bool ObjectClassProfiling();

struct ScopedNamed
{
    Table table;
    const char* key;
    u64 calls0;
    u64 bytes0;
    u64 frees0;
    u64 freeBytes0;

    ScopedNamed(Table t, const char* k)
        : table(t), key(k), calls0(AllocCallsThread()), bytes0(AllocBytesThread()),
          frees0(FreeCallsThread()), freeBytes0(FreeBytesThread())
    {
    }

    ~ScopedNamed()
    {
        AddNamed(table, key, AllocCallsThread() - calls0, AllocBytesThread() - bytes0,
            FreeCallsThread() - frees0, FreeBytesThread() - freeBytes0);
    }

    ScopedNamed(const ScopedNamed&) = delete;
    ScopedNamed& operator=(const ScopedNamed&) = delete;
};

// ------------------------------ size histogram -----------------------------
// Global pow2 size histogram (<=16B .. >256KB). Adds two atomic adds to every
// allocation while enabled, so it is off unless the overlay asks for it.

constexpr int histBucketCount = 16;

XRCORE_API void SetHistogramEnabled(bool enabled);
XRCORE_API bool HistogramEnabled();

// ------------------------ zone-targeted backtraces -------------------------
// Arm capture for one CPU-profiler zone id: every allocation made while that
// zone is the innermost open zone on its thread is backtraced and deduped by
// call site. Stays armed until a frame captures something (zones only record
// on profiler-sampled frames), then the report is finalized at FrameEnd.
// Supported on Apple/Linux (execinfo); ArmBacktraceCapture is a no-op
// elsewhere. Target noZone to catch allocations outside every zone.

constexpr u32 noZone = 0xffffffffu;

constexpr int btReportMaxSites = 64;
constexpr int btReportFramesPerSite = 16;
constexpr int btSymLen = 160;

struct BacktraceSite
{
    u64 count = 0;
    u64 bytes = 0;
    int depth = 0;
    char frames[btReportFramesPerSite][btSymLen] = {};
};

struct BacktraceReport
{
    bool ready = false;
    u32 zoneId = noZone;
    const char* zoneName = nullptr;
    int siteCount = 0;
    u64 totalCalls = 0;
    BacktraceSite sites[btReportMaxSites] = {};
};

XRCORE_API bool BacktraceCaptureSupported();
XRCORE_API void ArmBacktraceCapture(u32 zoneId, const char* zoneName); // zoneName must be static/interned
XRCORE_API void DisarmBacktraceCapture();
XRCORE_API bool BacktraceCaptureArmed();
XRCORE_API const BacktraceReport& GetBacktraceReport();

// Innermost open CPU zone on this thread; maintained by the CPU profiler so
// the allocation hook can match armed captures without calling back into it.
XRCORE_API void SetCurrentZone(u32 zoneId);
XRCORE_API u32 CurrentZone();

// --------------------------- enforcement scopes ----------------------------
// V8-style assert scope: while alive, any allocation through xrMemory on this
// thread is a violation - counted in the frame report and logged (first few
// per frame). AllowHeapAlloc re-permits inside a disallowed region. Both nest.

XRCORE_API const char* PushDisallow(const char* context); // returns previous context
XRCORE_API void PopDisallow(const char* prevContext);
XRCORE_API void PushAllow();
XRCORE_API void PopAllow();

struct DisallowHeapAlloc
{
    const char* prev;

    explicit DisallowHeapAlloc(const char* context) : prev(PushDisallow(context)) {}
    ~DisallowHeapAlloc() { PopDisallow(prev); }

    DisallowHeapAlloc(const DisallowHeapAlloc&) = delete;
    DisallowHeapAlloc& operator=(const DisallowHeapAlloc&) = delete;
};

struct AllowHeapAlloc
{
    AllowHeapAlloc() { PushAllow(); }
    ~AllowHeapAlloc() { PopAllow(); }

    AllowHeapAlloc(const AllowHeapAlloc&) = delete;
    AllowHeapAlloc& operator=(const AllowHeapAlloc&) = delete;
};

// ------------------------------ frame report -------------------------------
// "main" figures are the thread that calls FrameBegin/FrameEnd (the main
// thread); "all" figures aggregate every thread.

struct FrameReport
{
    bool valid = false;

    u64 mainCalls = 0;
    u64 mainBytes = 0;
    u64 mainFrees = 0;
    u64 mainFreeBytes = 0;

    u64 allCalls = 0;
    u64 allBytes = 0;
    u64 allFrees = 0;
    u64 allFreeBytes = 0;

    u64 avgMainCalls = 0;
    u64 avgMainBytes = 0;
    u64 peakMainCalls = 0;
    u64 peakMainBytes = 0;

    u64 disallowViolations = 0;

    u64 histCalls[histBucketCount] = {};
    u64 histBytes[histBucketCount] = {};

    NamedEntry named[tableCount][namedCapacity] = {};
    int namedUsed[tableCount] = {};
};

XRCORE_API void FrameBegin();
XRCORE_API void FrameEnd(bool record);
XRCORE_API const FrameReport& Report();

}
