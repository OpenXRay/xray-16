#include "stdafx.h"

#include "SDL.h"

#if defined(WINDOWS)
#include <Psapi.h>
#elif defined(LINUX)
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

// XXX: fix xrMemory_align on Linux
// and enable it
#ifdef WINDOWS
#define USE_XR_ALIGNED_MALLOC
#endif

// Define this if you want to use TBB allocator
//#define USE_TBB_MALLOC

#if defined(USE_XR_ALIGNED_MALLOC)
#include "Memory/xrMemory_align.h"
constexpr size_t xr_default_alignment = 16;

#define xr_internal_malloc(size) xr_aligned_malloc(size, xr_default_alignment)
#define xr_internal_realloc(ptr, size) xr_aligned_realloc(ptr, size, xr_default_alignment)
#define xr_internal_free(ptr) xr_aligned_free(ptr)
#elif defined(USE_TBB_MALLOC)
#include <tbb/scalable_allocator.h>

#define xr_internal_malloc(size) scalable_malloc(size)
#define xr_internal_realloc(ptr, size) scalable_realloc(ptr, size)
#define xr_internal_free(ptr) scalable_free(ptr)
#else
// Additional bytes of memory to hide memory problems on Release
// But for Debug we don't need this if we want to find these problems
#ifdef NDEBUG
constexpr size_t xr_reserved_tail = 8;
#else
constexpr size_t xr_reserved_tail = 0;
#endif

#define xr_internal_malloc(size) malloc(size + xr_reserved_tail)
#define xr_internal_realloc(ptr, size) realloc(ptr, size + xr_reserved_tail)
#define xr_internal_free(ptr) free(ptr)
#endif

xrMemory Memory;
// Also used in src\xrCore\xrDebug.cpp to prevent use of g_pStringContainer before it initialized
bool shared_str_initialized = false;

xrMemory::xrMemory()
{
}

void xrMemory::_initialize()
{
    stat_calls = 0;

    g_pStringContainer = new str_container();
    shared_str_initialized = true;
    g_pSharedMemoryContainer = new smem_container();
}

void xrMemory::_destroy()
{
    xr_delete(g_pSharedMemoryContainer);
    xr_delete(g_pStringContainer);
}

XRCORE_API void vminfo(size_t* _free, size_t* reserved, size_t* committed)
{
#if defined(WINDOWS)
    MEMORY_BASIC_INFORMATION memory_info;
    memory_info.BaseAddress = 0;
    *_free = *reserved = *committed = 0;
    while (VirtualQuery(memory_info.BaseAddress, &memory_info, sizeof(memory_info)))
    {
        switch (memory_info.State)
        {
        case MEM_FREE: *_free += memory_info.RegionSize; break;
        case MEM_RESERVE: *reserved += memory_info.RegionSize; break;
        case MEM_COMMIT: *committed += memory_info.RegionSize; break;
        }
        memory_info.BaseAddress = (char*)memory_info.BaseAddress + memory_info.RegionSize;
    }
#elif defined(LINUX)
    struct sysinfo si;
    sysinfo(&si);
    *_free = si.freeram * si.mem_unit;
    *reserved = si.bufferram * si.mem_unit;
    *committed = (si.totalram - si.freeram + si.totalswap - si.freeswap) * si.mem_unit;
#endif
}

XRCORE_API void log_vminfo()
{
    size_t w_free, w_reserved, w_committed;
    vminfo(&w_free, &w_reserved, &w_committed);
    Msg("* [ %s ]: free[%d K], reserved[%d K], committed[%d K]", SDL_GetPlatform(), w_free / 1024, w_reserved / 1024, w_committed / 1024);
}

size_t xrMemory::mem_usage()
{
#if defined(WINDOWS)
    PROCESS_MEMORY_COUNTERS pmc = {};
    if (HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId()))
    {
        GetProcessMemoryInfo(h, &pmc, sizeof(pmc));
        CloseHandle(h);
    }
    return pmc.PagefileUsage;
#elif defined(LINUX)
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (size_t)ru.ru_maxrss;
#endif
}

void xrMemory::mem_compact()
{
#if defined(WINDOWS)
    RegFlushKey(HKEY_CLASSES_ROOT);
    RegFlushKey(HKEY_CURRENT_USER);
#endif

    /*
    Следующие две команды в целом не нужны.
    Современные аллокаторы достаточно грамотно и когда нужно возвращают память операционной системе.
    Эта строчки нужны, скорее всего, в определённых ситуациях, вроде использования файлов отображаемых в память,
    которые требуют большие свободные области памяти.
    Но всё-же чистку tbb, возможно, стоит оставить. Но и это под большим вопросом.
    */
#ifdef USE_TBB_MALLOC
    scalable_allocation_command(TBBMALLOC_CLEAN_ALL_BUFFERS, nullptr);
#endif
    //HeapCompact(GetProcessHeap(), 0);
    if (g_pStringContainer)
        g_pStringContainer->clean();
    if (g_pSharedMemoryContainer)
        g_pSharedMemoryContainer->clean();

#if defined(WINDOWS)
    if (strstr(Core.Params, "-swap_on_compact"))
        SetProcessWorkingSetSize(GetCurrentProcess(), size_t(-1), size_t(-1));
#endif
}

void* xrMemory::mem_alloc(size_t size)
{
    stat_calls++;
    return xr_internal_malloc(size);
}

void* xrMemory::mem_realloc(void* ptr, size_t size)
{
    stat_calls++;
    return xr_internal_realloc(ptr, size);
}

void xrMemory::mem_free(void* ptr)
{
    stat_calls++;
    xr_internal_free(ptr);
}

// xr_strdup
XRCORE_API pstr xr_strdup(pcstr string)
{
    VERIFY(string);
    size_t len = xr_strlen(string) + 1;
    char* memory = (char*)xr_malloc(len);
    CopyMemory(memory, string, len);
    return memory;
}
