#include "stdafx.h"

#include "MemoryStats.h"
#include "log.h"

#include <atomic>

#if defined(XR_PLATFORM_APPLE) || defined(XR_PLATFORM_LINUX)
#define XR_MEMSTATS_BACKTRACE 1
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#else
#define XR_MEMSTATS_BACKTRACE 0
#endif

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

    thread_local u32 tl_currentZone = noZone;
    thread_local int tl_targetDepth = 0;
    thread_local u32 tl_targetGen = 0;
    thread_local bool tl_btReentry = false;

    std::atomic<bool> g_btArmed{ false };
    std::atomic<u32> g_btGen{ 0 };
    u32 g_btTargetZone = noZone;
    const char* g_btZoneName = nullptr;
    BacktraceReport g_btReport;

#if XR_MEMSTATS_BACKTRACE
    constexpr int btMaxFrames = 32;
    constexpr int btMaxSites = 512;

    struct BtCapture
    {
        u64 hash;
        u64 count;
        u64 bytes;
        int depth;
        void* frames[btMaxFrames];
    };

    std::atomic_flag g_btLock = ATOMIC_FLAG_INIT;
    BtCapture g_btCapture[btMaxSites];
    int g_btCaptureCount = 0;

    struct BtGuard
    {
        BtGuard() { while (g_btLock.test_and_set(std::memory_order_acquire)) {} }
        ~BtGuard() { g_btLock.clear(std::memory_order_release); }
    };

    void CaptureBacktrace(size_t size)
    {
        tl_btReentry = true;

        void* frames[btMaxFrames];
        const int depth = backtrace(frames, btMaxFrames);

        u64 hash = 1469598103934665603ull;
        for (int i = 0; i < depth; ++i)
        {
            hash ^= reinterpret_cast<u64>(frames[i]);
            hash *= 1099511628211ull;
        }

        {
            BtGuard guard;
            int slot = -1;
            for (int i = 0; i < g_btCaptureCount; ++i)
            {
                if (g_btCapture[i].hash == hash)
                {
                    slot = i;
                    break;
                }
            }
            if (slot < 0 && g_btCaptureCount < btMaxSites)
            {
                slot = g_btCaptureCount++;
                g_btCapture[slot].hash = hash;
                g_btCapture[slot].count = 0;
                g_btCapture[slot].bytes = 0;
                g_btCapture[slot].depth = depth;
                for (int i = 0; i < depth; ++i)
                    g_btCapture[slot].frames[i] = frames[i];
            }
            if (slot >= 0)
            {
                g_btCapture[slot].count += 1;
                g_btCapture[slot].bytes += size;
            }
        }

        tl_btReentry = false;
    }

    void SymbolicateFrame(void* addr, char* out, size_t outLen)
    {
        Dl_info info;
        if (dladdr(addr, &info) && info.dli_sname)
        {
            int status = 0;
            char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
            const char* name = (status == 0 && demangled) ? demangled : info.dli_sname;
            std::snprintf(out, outLen, "%s", name);
            if (demangled)
                free(demangled);
        }
        else
        {
            std::snprintf(out, outLen, "%p", addr);
        }
    }

    bool MachineryFrame(const char* sym)
    {
        return std::strstr(sym, "memstats") != nullptr
            || std::strstr(sym, "xrMemory::") != nullptr
            || std::strncmp(sym, "operator new", 12) == 0
            || std::strstr(sym, "xr_malloc") != nullptr
            || std::strstr(sym, "xr_realloc") != nullptr
            || std::strstr(sym, "xr_strdup") != nullptr
            || std::strstr(sym, "__libcpp_") != nullptr
            || std::strstr(sym, "allocate_at_least") != nullptr
            || std::strstr(sym, "std::__1::allocator<") != nullptr;
    }

    void FinalizeBacktrace()
    {
        if (!g_btArmed.load(std::memory_order_relaxed))
            return;

        static BtCapture sites[btReportMaxSites];
        int n = 0;

        {
            BtGuard guard;
            if (g_btCaptureCount == 0)
                return;

            for (int i = 0; i < g_btCaptureCount; ++i)
                for (int j = i + 1; j < g_btCaptureCount; ++j)
                    if (g_btCapture[j].count > g_btCapture[i].count)
                    {
                        BtCapture tmp = g_btCapture[i];
                        g_btCapture[i] = g_btCapture[j];
                        g_btCapture[j] = tmp;
                    }

            n = g_btCaptureCount < btReportMaxSites ? g_btCaptureCount : btReportMaxSites;
            for (int i = 0; i < n; ++i)
                sites[i] = g_btCapture[i];

            g_btReport.zoneId = g_btTargetZone;
            g_btReport.zoneName = g_btZoneName;
            g_btArmed.store(false, std::memory_order_relaxed);
            g_btCaptureCount = 0;
        }

        g_btReport.siteCount = n;
        g_btReport.totalCalls = 0;

        for (int i = 0; i < n; ++i)
        {
            const BtCapture& cap = sites[i];
            BacktraceSite& site = g_btReport.sites[i];
            site.count = cap.count;
            site.bytes = cap.bytes;
            g_btReport.totalCalls += cap.count;

            char sym[btSymLen];
            int first = 0;
            while (first < cap.depth)
            {
                SymbolicateFrame(cap.frames[first], sym, btSymLen);
                if (!MachineryFrame(sym))
                    break;
                ++first;
            }
            if (first >= cap.depth)
                first = 0;

            const int avail = cap.depth - first;
            const int frames = avail < btReportFramesPerSite ? avail : btReportFramesPerSite;
            site.depth = frames;
            for (int f = 0; f < frames; ++f)
                SymbolicateFrame(cap.frames[first + f], site.frames[f], btSymLen);
        }

        g_btReport.ready = true;
    }
#else
    void FinalizeBacktrace() {}
#endif
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

#if XR_MEMSTATS_BACKTRACE
    if (g_btArmed.load(std::memory_order_relaxed) && !tl_btReentry)
    {
        const bool inTargetSubtree = tl_targetDepth > 0
            && tl_targetGen == g_btGen.load(std::memory_order_relaxed);
        if (inTargetSubtree || (g_btTargetZone == noZone && tl_currentZone == noZone))
            CaptureBacktrace(size);
    }
#endif

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

    FinalizeBacktrace();
}

XRCORE_API const FrameReport& Report() { return g_report; }

XRCORE_API void ZoneEntered(u32 zoneId)
{
    tl_currentZone = zoneId;
    if (!g_btArmed.load(std::memory_order_relaxed))
        return;
    const u32 gen = g_btGen.load(std::memory_order_relaxed);
    if (tl_targetGen != gen)
    {
        tl_targetGen = gen;
        tl_targetDepth = 0;
    }
    if (zoneId == g_btTargetZone)
        ++tl_targetDepth;
}

XRCORE_API void ZoneExited(u32 zoneId, u32 currentZoneId)
{
    tl_currentZone = currentZoneId;
    if (!g_btArmed.load(std::memory_order_relaxed))
        return;
    if (tl_targetGen == g_btGen.load(std::memory_order_relaxed)
        && zoneId == g_btTargetZone && tl_targetDepth > 0)
        --tl_targetDepth;
}

XRCORE_API u32 CurrentZone() { return tl_currentZone; }

XRCORE_API bool BacktraceCaptureSupported() { return XR_MEMSTATS_BACKTRACE != 0; }

XRCORE_API void ArmBacktraceCapture(u32 zoneId, const char* zoneName)
{
#if XR_MEMSTATS_BACKTRACE
    BtGuard guard;
    g_btCaptureCount = 0;
    g_btTargetZone = zoneId;
    g_btZoneName = zoneName;
    g_btReport.ready = false;
    g_btGen.fetch_add(1, std::memory_order_relaxed);
    g_btArmed.store(true, std::memory_order_relaxed);
#else
    (void)zoneId;
    (void)zoneName;
    Msg("! memstats: backtrace capture is not supported on this platform");
#endif
}

XRCORE_API void DisarmBacktraceCapture()
{
#if XR_MEMSTATS_BACKTRACE
    BtGuard guard;
    g_btCaptureCount = 0;
    g_btGen.fetch_add(1, std::memory_order_relaxed);
#endif
    g_btArmed.store(false, std::memory_order_relaxed);
}

XRCORE_API bool BacktraceCaptureArmed() { return g_btArmed.load(std::memory_order_relaxed); }

XRCORE_API const char* ArmedZoneName()
{
    return g_btArmed.load(std::memory_order_relaxed) ? g_btZoneName : nullptr;
}

XRCORE_API const BacktraceReport& GetBacktraceReport() { return g_btReport; }

}
