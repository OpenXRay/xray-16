#include "stdafx.h"
#include "xrCore.h"

#ifdef DEBUG_MEMORY_MANAGER
# pragma warning(push)
# pragma warning(disable:4995)
# include <malloc.h>
# pragma warning(pop)

static bool g_mem_alloc_gather_stats = false;
static float g_mem_alloc_gather_stats_frequency = 0.f;

typedef std::pair<PSTR,u32> STATS_PAIR;
typedef std::multimap<u32,STATS_PAIR> STATS;
static STATS stats;

void mem_alloc_gather_stats (const bool& value)
{
    g_mem_alloc_gather_stats = value;
}

void mem_alloc_gather_stats_frequency (const float& value)
{
    g_mem_alloc_gather_stats_frequency = value;
}

void mem_alloc_show_stats ()
{
    u32 size = (u32)stats.size();
    STATS_PAIR* strings = (STATS_PAIR*)_alloca(size*sizeof(STATS_PAIR));
    STATS_PAIR* e = strings + size;
    STATS_PAIR* i = strings;

    u32 accumulator = 0;
    STATS::const_iterator I = stats.begin();
    STATS::const_iterator E = stats.end();
    for ( ; I != E; ++I, ++i)
    {
        *i = (*I).second;
        accumulator += (*I).second.second;
    }

    struct predicate
    {
        static inline bool compare (const STATS_PAIR& _0, const STATS_PAIR& _1)
        {
            return (_0.second < _1.second);
        }
    };

    std::sort (strings,e,predicate::compare);

    int j = 0;
    for (i = strings; i != e; ++i, ++j)
    {
        Msg ("%d(%d)-----------------%d[%d]:%5.2f%%------------------",j,size,(*i).second,accumulator,((*i).second*100)/float(accumulator));
        Log ((*i).first);
    }
}

void mem_alloc_clear_stats ()
{
    STATS::iterator I = stats.begin();
    STATS::iterator E = stats.end();
    for ( ; I != E; ++I)
        free ((*I).second.first);

    stats.clear ();
}

namespace
{
StackTraceInfo StackTrace;
}

__declspec(noinline)
void save_stack_trace ()
{
    if (!g_mem_alloc_gather_stats)
        return;

    if (::Random.randF() >= g_mem_alloc_gather_stats_frequency)
        return;

    // OutputDebugStackTrace ("----------------------------------------------------");
    StackTrace.Count = xrDebug::BuildStackTrace(StackTrace.Frames, StackTrace.Capacity, StackTrace.LineCapacity);
    const int skipFrames = 2;
    if (StackTrace.Count<=skipFrames)
        return;
    u32 accumulator = 0;
    int* lengths = (int*)_alloca((StackTrace.Count-skipFrames)*sizeof(int));
    {
        int* I = lengths;
        for (int i = skipFrames; i<StackTrace.Count; ++i, ++I)
        {
            *I = xr_strlen(StackTrace[i]);
            accumulator += u32(*I+1);
        }
    }

    PSTR string = (PSTR)malloc(accumulator);
    {
        PSTR J = string;
        int* I = lengths;
        for (int i = skipFrames; i<StackTrace.Count; ++i, ++I, ++J)
        {
            memcpy(J, StackTrace[i], *I);
            J += *I;
            *J = '\n';
        }
        *--J = 0;
    }

    u32 crc = crc32(string, accumulator);
    STATS::iterator I = stats.find(crc);
    STATS::iterator E = stats.end();
    for ( ; I != E; ++I)
    {
        if ((*I).first != crc)
            break;

        if (xr_strcmp((*I).second.first,string))
            continue;

        ++((*I).second.second);
        return;
    }

    stats.insert(std::make_pair(crc, std::make_pair(string, 1)));
}
#endif // DEBUG