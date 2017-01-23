#include "stdafx.h"

#ifndef DEBUG_MEMORY_MANAGER
#include <malloc.h> // _alloca

namespace
{
bool StatsGatherEnabled = false;
float StatsGatherFrequency = 0.0f;
StackTraceInfo StackTrace;
}

struct StatsItem
{
    char *StackTrace;
    size_t Calls;
};

static std::multimap<u32, StatsItem> stats;

void mem_alloc_gather_stats(const bool &value)
{ StatsGatherEnabled = value; }

void mem_alloc_gather_stats_frequency(const float &value)
{ StatsGatherFrequency = value; }

void mem_alloc_show_stats()
{
    size_t statsSize = stats.size();
    auto strings = (StatsItem *)malloc(statsSize*sizeof(StatsItem));
    size_t totalCalls = 0, i = 0;
    for (auto &pair : stats)
    {
        strings[i] = pair.second;
        i++;
        totalCalls += pair.second.Calls;
    }
    struct Comparer
    {
        bool operator()(const StatsItem &a, const StatsItem &b)
        { return a.Calls<b.Calls; }
    };
    std::sort(strings, strings+statsSize, Comparer());
    for (i = 0; i<statsSize; i++)
    {
        StatsItem &item = strings[i];
        Msg("%zu(%zu)-----------------%zu[%zu]:%5.2f%%------------------",
            i, statsSize, item.Calls, totalCalls, item.Calls*100/float(totalCalls));
        Log(item.StackTrace);
    }
    free(strings);
}

void mem_alloc_clear_stats()
{
    for (auto &item : stats)
        free(item.second.StackTrace);
    stats.clear();
}

NO_INLINE void save_stack_trace()
{
    if (!StatsGatherEnabled)
        return;
    if (::Random.randF()>=StatsGatherFrequency)
        return;
    StackTrace.Count = xrDebug::BuildStackTrace(StackTrace.Frames, StackTrace.Capacity, StackTrace.LineCapacity);
    const size_t skipFrames = 2;
    if (StackTrace.Count<=skipFrames)
        return;
    size_t frameCount = StackTrace.Count-skipFrames;
    size_t totalSize = 0;
    auto lengths = (size_t *)_alloca(frameCount*sizeof(size_t));
    for (size_t i = 0; i<frameCount; i++)
    {
        lengths[i] = strlen(StackTrace[i+skipFrames]);
        totalSize += lengths[i]+1;
    }
    char *stackTrace = (char *)malloc(totalSize);
    {
        char *ptr = stackTrace;
        for (size_t i = 0; i<frameCount; i++)
        {
            memcpy(ptr, StackTrace[i], lengths[i]);
            ptr += lengths[i];
            *ptr = '\n';
        }
        *ptr = 0;
    }
    u32 crc = crc32(stackTrace, totalSize);
    for (auto it = stats.find(crc); it!=stats.end(); it++)
    {
        auto &pair = *it;
        if (pair.first!=crc)
            break;
        if (strcmp(pair.second.StackTrace, stackTrace))
            continue;
        pair.second.Calls++;
        return;
    }
    stats.insert({crc, {stackTrace, 1}});
}
#endif // DEBUG_MEMORY_MANAGER
