#include "stdafx.h"

#include "MemoryStats.h"
#include "log.h"

#include <atomic>

namespace xray::memstats
{

namespace
{
    std::atomic<u64> g_allocCalls{ 0 };
    std::atomic<u64> g_allocBytes{ 0 };
    std::atomic<u64> g_freeCalls{ 0 };
    std::atomic<u64> g_freeBytes{ 0 };
    thread_local u64 tl_allocCalls = 0;
    thread_local u64 tl_allocBytes = 0;
    thread_local u64 tl_freeCalls = 0;
    thread_local u64 tl_freeBytes = 0;

    std::atomic<bool> g_histogramEnabled{ false };
    std::atomic<u64> g_histCalls[histBucketCount];
    std::atomic<u64> g_histBytes[histBucketCount];

    std::atomic_flag g_namedLock = ATOMIC_FLAG_INIT;
    NamedEntry g_named[tableCount][namedCapacity];
    int g_namedUsed[tableCount] = {};

    u64 g_frameAllCalls0 = 0;
    u64 g_frameAllBytes0 = 0;
    u64 g_frameMainCalls0 = 0;
    u64 g_frameMainBytes0 = 0;
    u64 g_frameAllFrees0 = 0;
    u64 g_frameAllFreeBytes0 = 0;
    u64 g_frameMainFrees0 = 0;
    u64 g_frameMainFreeBytes0 = 0;

    constexpr int avgWindow = 120;
    u64 g_callsRing[avgWindow] = {};
    u64 g_bytesRing[avgWindow] = {};
    int g_ringPos = 0;
    int g_ringFilled = 0;
    u64 g_callsRingSum = 0;
    u64 g_bytesRingSum = 0;
    u64 g_peakMainCalls = 0;
    u64 g_peakMainBytes = 0;

    bool g_objectClassProfiling = false;

    FrameReport g_report;

    struct SpinGuard
    {
        SpinGuard() { while (g_namedLock.test_and_set(std::memory_order_acquire)) {} }
        ~SpinGuard() { g_namedLock.clear(std::memory_order_release); }
    };

    int HistBucket(size_t size)
    {
        int b = 0;
        u64 threshold = 16;
        while (b < histBucketCount - 1 && size > threshold)
        {
            threshold <<= 1;
            ++b;
        }
        return b;
    }

    // Disallow scope state. The violation path may itself allocate (logging),
    // so it is guarded against re-entry; those allocations are still counted,
    // just not re-reported.
    thread_local int tl_disallowDepth = 0;
    thread_local int tl_allowDepth = 0;
    thread_local const char* tl_disallowContext = nullptr;
    thread_local bool tl_inViolationReport = false;
    std::atomic<u64> g_frameViolations{ 0 };
    std::atomic<u64> g_frameViolationsLogged{ 0 };
    constexpr u64 maxViolationLogsPerFrame = 16;

    void ReportViolation(size_t size)
    {
        tl_inViolationReport = true;
        g_frameViolations.fetch_add(1, std::memory_order_relaxed);
        if (g_frameViolationsLogged.fetch_add(1, std::memory_order_relaxed) < maxViolationLogsPerFrame)
        {
            Msg("! memstats: %zu byte alloc inside DisallowHeapAlloc(%s)",
                size, tl_disallowContext ? tl_disallowContext : "?");
        }
        tl_inViolationReport = false;
    }
}

XRCORE_API void CountAlloc(size_t size)
{
    g_allocCalls.fetch_add(1, std::memory_order_relaxed);
    g_allocBytes.fetch_add(size, std::memory_order_relaxed);
    tl_allocCalls += 1;
    tl_allocBytes += size;

    if (g_histogramEnabled.load(std::memory_order_relaxed))
    {
        const int bucket = HistBucket(size);
        g_histCalls[bucket].fetch_add(1, std::memory_order_relaxed);
        g_histBytes[bucket].fetch_add(size, std::memory_order_relaxed);
    }

    if (tl_disallowDepth > 0 && tl_allowDepth == 0 && !tl_inViolationReport)
        ReportViolation(size);
}

XRCORE_API void CountFree(size_t size)
{
    g_freeCalls.fetch_add(1, std::memory_order_relaxed);
    g_freeBytes.fetch_add(size, std::memory_order_relaxed);
    tl_freeCalls += 1;
    tl_freeBytes += size;
}

XRCORE_API u64 AllocCallsTotal() { return g_allocCalls.load(std::memory_order_relaxed); }
XRCORE_API u64 AllocBytesTotal() { return g_allocBytes.load(std::memory_order_relaxed); }
XRCORE_API u64 AllocCallsThread() { return tl_allocCalls; }
XRCORE_API u64 AllocBytesThread() { return tl_allocBytes; }
XRCORE_API u64 FreeCallsThread() { return tl_freeCalls; }
XRCORE_API u64 FreeBytesThread() { return tl_freeBytes; }

XRCORE_API void SetObjectClassProfiling(bool enabled) { g_objectClassProfiling = enabled; }
XRCORE_API bool ObjectClassProfiling() { return g_objectClassProfiling; }

XRCORE_API void SetHistogramEnabled(bool enabled) { g_histogramEnabled.store(enabled, std::memory_order_relaxed); }
XRCORE_API bool HistogramEnabled() { return g_histogramEnabled.load(std::memory_order_relaxed); }

XRCORE_API void AddNamed(Table table, const char* key, u64 calls, u64 bytes, u64 frees, u64 freeBytes)
{
    if ((calls == 0 && bytes == 0 && frees == 0) || key == nullptr)
        return;

    const int t = static_cast<int>(table);
    SpinGuard guard;

    NamedEntry* entries = g_named[t];
    int& used = g_namedUsed[t];

    for (int i = 0; i < used; ++i)
    {
        if (entries[i].key == key)
        {
            entries[i].calls += calls;
            entries[i].bytes += bytes;
            entries[i].frees += frees;
            entries[i].freeBytes += freeBytes;
            return;
        }
    }

    if (used < namedCapacity)
    {
        entries[used].key = key;
        entries[used].calls = calls;
        entries[used].bytes = bytes;
        entries[used].frees = frees;
        entries[used].freeBytes = freeBytes;
        ++used;
    }
}

XRCORE_API const char* PushDisallow(const char* context)
{
    const char* prev = tl_disallowContext;
    tl_disallowContext = context;
    ++tl_disallowDepth;
    return prev;
}

XRCORE_API void PopDisallow(const char* prevContext)
{
    --tl_disallowDepth;
    tl_disallowContext = prevContext;
}

XRCORE_API void PushAllow() { ++tl_allowDepth; }
XRCORE_API void PopAllow() { --tl_allowDepth; }

XRCORE_API void FrameBegin()
{
    g_frameAllCalls0 = AllocCallsTotal();
    g_frameAllBytes0 = AllocBytesTotal();
    g_frameMainCalls0 = tl_allocCalls;
    g_frameMainBytes0 = tl_allocBytes;
    g_frameAllFrees0 = g_freeCalls.load(std::memory_order_relaxed);
    g_frameAllFreeBytes0 = g_freeBytes.load(std::memory_order_relaxed);
    g_frameMainFrees0 = tl_freeCalls;
    g_frameMainFreeBytes0 = tl_freeBytes;

    g_frameViolations.store(0, std::memory_order_relaxed);
    g_frameViolationsLogged.store(0, std::memory_order_relaxed);

    for (int b = 0; b < histBucketCount; ++b)
    {
        g_histCalls[b].store(0, std::memory_order_relaxed);
        g_histBytes[b].store(0, std::memory_order_relaxed);
    }

    SpinGuard guard;
    for (int t = 0; t < tableCount; ++t)
        g_namedUsed[t] = 0;
}

XRCORE_API void FrameEnd(bool record)
{
    if (!record)
        return;

    const u64 mainCalls = tl_allocCalls - g_frameMainCalls0;
    const u64 mainBytes = tl_allocBytes - g_frameMainBytes0;
    const u64 allCalls = AllocCallsTotal() - g_frameAllCalls0;
    const u64 allBytes = AllocBytesTotal() - g_frameAllBytes0;
    const u64 mainFrees = tl_freeCalls - g_frameMainFrees0;
    const u64 mainFreeBytes = tl_freeBytes - g_frameMainFreeBytes0;
    const u64 allFrees = g_freeCalls.load(std::memory_order_relaxed) - g_frameAllFrees0;
    const u64 allFreeBytes = g_freeBytes.load(std::memory_order_relaxed) - g_frameAllFreeBytes0;

    g_callsRingSum -= g_callsRing[g_ringPos];
    g_bytesRingSum -= g_bytesRing[g_ringPos];
    g_callsRing[g_ringPos] = mainCalls;
    g_bytesRing[g_ringPos] = mainBytes;
    g_callsRingSum += mainCalls;
    g_bytesRingSum += mainBytes;
    g_ringPos = (g_ringPos + 1) % avgWindow;
    if (g_ringFilled < avgWindow)
        ++g_ringFilled;

    if (mainCalls > g_peakMainCalls)
        g_peakMainCalls = mainCalls;
    if (mainBytes > g_peakMainBytes)
        g_peakMainBytes = mainBytes;

    g_report.valid = true;
    g_report.mainCalls = mainCalls;
    g_report.mainBytes = mainBytes;
    g_report.mainFrees = mainFrees;
    g_report.mainFreeBytes = mainFreeBytes;
    g_report.allCalls = allCalls;
    g_report.allBytes = allBytes;
    g_report.allFrees = allFrees;
    g_report.allFreeBytes = allFreeBytes;
    g_report.avgMainCalls = g_ringFilled ? g_callsRingSum / static_cast<u64>(g_ringFilled) : 0;
    g_report.avgMainBytes = g_ringFilled ? g_bytesRingSum / static_cast<u64>(g_ringFilled) : 0;
    g_report.peakMainCalls = g_peakMainCalls;
    g_report.peakMainBytes = g_peakMainBytes;
    g_report.disallowViolations = g_frameViolations.load(std::memory_order_relaxed);

    for (int b = 0; b < histBucketCount; ++b)
    {
        g_report.histCalls[b] = g_histCalls[b].load(std::memory_order_relaxed);
        g_report.histBytes[b] = g_histBytes[b].load(std::memory_order_relaxed);
    }

    SpinGuard guard;
    for (int t = 0; t < tableCount; ++t)
    {
        const int used = g_namedUsed[t];
        g_report.namedUsed[t] = used;
        for (int i = 0; i < used; ++i)
            g_report.named[t][i] = g_named[t][i];
    }
}

XRCORE_API const FrameReport& Report() { return g_report; }

}
