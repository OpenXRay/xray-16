#pragma once

#include "xr_types.h"

#include <cstddef>

namespace xray::memstats
{

XRCORE_API void CountAlloc(size_t size);
XRCORE_API void CountFree(size_t size);

XRCORE_API u64 AllocCallsTotal();
XRCORE_API u64 AllocBytesTotal();
XRCORE_API u64 AllocCallsThread();
XRCORE_API u64 AllocBytesThread();
XRCORE_API u64 FreeCallsThread();
XRCORE_API u64 FreeBytesThread();

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

constexpr int histBucketCount = 16;

XRCORE_API void SetHistogramEnabled(bool enabled);
XRCORE_API bool HistogramEnabled();

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
XRCORE_API void ArmBacktraceCapture(u32 zoneId, const char* zoneName);
XRCORE_API void DisarmBacktraceCapture();
XRCORE_API bool BacktraceCaptureArmed();
XRCORE_API const BacktraceReport& GetBacktraceReport();

XRCORE_API void SetCurrentZone(u32 zoneId);
XRCORE_API u32 CurrentZone();

XRCORE_API const char* PushDisallow(const char* context);
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
