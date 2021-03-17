#include "stdafx.h"

#include "SDL.h"

#if defined(XR_PLATFORM_WINDOWS)
#include <Psapi.h>
#elif defined(XR_PLATFORM_LINUX)
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

// On other platforms these options are controlled by CMake
#if defined(XR_PLATFORM_WINDOWS)
#  define USE_MIMALLOC
//#  define USE_PURE_ALLOC
#endif

#if defined(USE_MIMALLOC)
#include "mimalloc.h"
#define xr_internal_malloc(size, alignment) mi_malloc_aligned(size, alignment);
#define xr_internal_malloc_nothrow(size, alignment) mi_malloc_aligned(size, alignment);
#define xr_internal_realloc(ptr, size, alignment) mi_realloc_aligned(ptr, size, alignment);
#define xr_internal_free(ptr, alignment) mi_free_aligned(ptr, alignment);
#elif defined(USE_XR_ALIGNED_MALLOC)
#include "Memory/xrMemory_align.h"
#define xr_internal_malloc(size, alignment) xr_aligned_malloc(size, alignment)
#define xr_internal_malloc_nothrow(size, alignment) xr_aligned_malloc(size, alignment)
#define xr_internal_realloc(ptr, size, alignment) xr_aligned_realloc(ptr, size, alignment)
#define xr_internal_free(ptr, alignment) xr_aligned_free(ptr)
#elif defined(USE_PURE_ALLOC)
// Additional bytes of memory to hide memory problems on Release
// But for Debug we don't need this if we want to find these problems
#ifdef NDEBUG
constexpr size_t xr_reserved_tail = 8;
#else
constexpr size_t xr_reserved_tail = 0;
#endif

#define xr_internal_malloc(size, alignment) malloc(size + xr_reserved_tail)
#define xr_internal_malloc_nothrow(size, alignment) malloc(size + xr_reserved_tail)
#define xr_internal_realloc(ptr, size, alignment) realloc(ptr, size + xr_reserved_tail)
#define xr_internal_free(ptr, alignment) free(ptr)
#else
#error Please, define explicitly which allocator you want to use
#endif

constexpr size_t DEFAULT_ALIGNMENT = 16;

xrMemory Memory;
// Also used in src\xrCore\xrDebug.cpp to prevent use of g_pStringContainer before it initialized
bool shared_str_initialized = false;

xrMemory::xrMemory()
{
}

void xrMemory::_initialize()
{
    stat_calls = 0;

    g_pStringContainer = xr_new<str_container>();
    shared_str_initialized = true;
    g_pSharedMemoryContainer = xr_new<smem_container>();
}

void xrMemory::_destroy()
{
    xr_delete(g_pSharedMemoryContainer);
    xr_delete(g_pStringContainer);
}

XRCORE_API void vminfo(size_t* _free, size_t* reserved, size_t* committed)
{
#if defined(XR_PLATFORM_WINDOWS)
    MEMORY_BASIC_INFORMATION memory_info;
    memory_info.BaseAddress = nullptr;
    *_free = *reserved = *committed = 0;
    while (VirtualQuery(memory_info.BaseAddress, &memory_info, sizeof(memory_info))) //-V575
    {
        switch (memory_info.State)
        {
        case MEM_FREE: *_free += memory_info.RegionSize; break;
        case MEM_RESERVE: *reserved += memory_info.RegionSize; break;
        case MEM_COMMIT: *committed += memory_info.RegionSize; break;
        }
        memory_info.BaseAddress = (char*)memory_info.BaseAddress + memory_info.RegionSize;
    }
#elif defined(XR_PLATFORM_LINUX)
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
#if defined(XR_PLATFORM_WINDOWS)
    PROCESS_MEMORY_COUNTERS pmc = {};
    if (HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId()))
    {
        GetProcessMemoryInfo(h, &pmc, sizeof(pmc));
        CloseHandle(h);
    }
    return pmc.PagefileUsage;
#elif defined(XR_PLATFORM_LINUX)
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (size_t)ru.ru_maxrss;
#endif
}

void xrMemory::mem_compact()
{
#if defined(XR_PLATFORM_WINDOWS)
    RegFlushKey(HKEY_CLASSES_ROOT);
    RegFlushKey(HKEY_CURRENT_USER);
#endif

    /*
    Следующая команда, в целом, не нужна.
    Современные аллокаторы достаточно грамотно и когда нужно возвращают память операционной системе.
    Эта строчка нужна, скорее всего, в определённых ситуациях, вроде использования файлов отображаемых в память,
    которые требуют большие свободные области памяти.
    */
    //HeapCompact(GetProcessHeap(), 0);
    if (g_pStringContainer)
        g_pStringContainer->clean();
    if (g_pSharedMemoryContainer)
        g_pSharedMemoryContainer->clean();

#if defined(XR_PLATFORM_WINDOWS)
    if (strstr(Core.Params, "-swap_on_compact"))
        SetProcessWorkingSetSize(GetCurrentProcess(), size_t(-1), size_t(-1));
#endif
}

void* xrMemory::mem_alloc(size_t size)
{
    stat_calls++;
    return xr_internal_malloc(size, DEFAULT_ALIGNMENT);
}

void* xrMemory::mem_alloc(size_t size, size_t alignment)
{
    stat_calls++;
    return xr_internal_malloc(size, alignment);
}

void* xrMemory::mem_alloc(size_t size, const std::nothrow_t&) noexcept
{
    stat_calls++;
    return xr_internal_malloc_nothrow(size, DEFAULT_ALIGNMENT);
}

void* xrMemory::mem_alloc(size_t size, size_t alignment, const std::nothrow_t&) noexcept
{
    stat_calls++;
    return xr_internal_malloc_nothrow(size, alignment);
}

void* xrMemory::mem_realloc(void* ptr, size_t size)
{
    stat_calls++;
    return xr_internal_realloc(ptr, size, DEFAULT_ALIGNMENT);
}

void* xrMemory::mem_realloc(void* ptr, size_t size, size_t alignment)
{
    stat_calls++;
    return xr_internal_realloc(ptr, size, alignment);
}

void xrMemory::mem_free(void* ptr)
{
    stat_calls++;
    xr_internal_free(ptr, DEFAULT_ALIGNMENT);
}

void xrMemory::mem_free(void* ptr, size_t alignment)
{
    stat_calls++;
    xr_internal_free(ptr, alignment);
}

// xr_strdup
XRCORE_API pstr xr_strdup(pcstr string)
{
#ifdef USE_MIMALLOC
    return mi_strdup(string);
#else
    VERIFY(string);
    size_t len = xr_strlen(string) + 1;
    char* memory = (char*)xr_malloc(len);
    CopyMemory(memory, string, len);
    return memory;
#endif
}
