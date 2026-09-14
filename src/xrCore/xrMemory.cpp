#include "stdafx.h"

#include <SDL3/SDL.h>

#include "MemoryStats.h"

#if defined(XR_PLATFORM_WINDOWS)
#include <Psapi.h>
#elif defined(XR_PLATFORM_LINUX)
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/resource.h>
#elif defined(XR_PLATFORM_BSD)
#include <sys/time.h>
#include <sys/resource.h>
#elif defined(XR_PLATFORM_HAIKU)
#include <OS.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

// On other platforms these options are controlled by CMake
#if defined(XR_PLATFORM_WINDOWS)
#   ifdef _DEBUG
#       define USE_PURE_ALLOC
#   else
#       define USE_MIMALLOC
#   endif
#endif

#if defined(USE_MIMALLOC)
    #include "mimalloc.h"

    static_assert(xrMemory::SMALL_SIZE_MAX <= MI_SMALL_SIZE_MAX, "Please, adjust SMALL_SIZE_ALLOC_MAX");

    #define xr_internal_malloc(size) mi_malloc(size)
    #define xr_internal_malloc_aligned(size, alignment) mi_malloc_aligned(size, alignment)
    #define xr_internal_malloc_nothrow(size) mi_malloc(size)
    #define xr_internal_malloc_nothrow_aligned(size, alignment) mi_malloc_aligned(size, alignment)
    #define xr_internal_small_alloc(size) mi_malloc_small(size)
    #define xr_internal_small_free(ptr) mi_free(ptr)

    #define xr_internal_realloc(ptr, size) mi_realloc(ptr, size)
    #define xr_internal_realloc_aligned(ptr, size, alignment) mi_realloc_aligned(ptr, size, alignment)

    #define xr_internal_free(ptr) mi_free(ptr)
    #define xr_internal_free_size(ptr, size) mi_free_size(ptr, size)
    #define xr_internal_free_aligned(ptr, alignment) mi_free_aligned(ptr, alignment)
    #define xr_internal_free_size_aligned(ptr, size, alignment) mi_free_size_aligned(ptr, size, alignment)

    #define xr_counted_alloc_size(ptr, size) mi_usable_size(ptr)
    #define xr_counted_free_size(ptr) mi_usable_size(ptr)
    #define xr_counted_alloc_size_aligned(ptr, size) mi_usable_size(ptr)
    #define xr_counted_free_size_aligned(ptr) mi_usable_size(ptr)
#elif defined(USE_PURE_ALLOC)
    #include <cerrno>
    #include <cstdlib>
    #include <cstring>

    #ifdef NDEBUG
        constexpr size_t xr_reserved_tail = 8;
    #else
        constexpr size_t xr_reserved_tail = 0;
    #endif

    #if defined(XR_PLATFORM_APPLE)
        #include <malloc/malloc.h>
        #define xr_counted_alloc_size(ptr, size) malloc_size(ptr)
        #define xr_counted_free_size(ptr) malloc_size(ptr)
    #elif defined(XR_PLATFORM_LINUX)
        #include <malloc.h>
        #define xr_counted_alloc_size(ptr, size) malloc_usable_size(ptr)
        #define xr_counted_free_size(ptr) malloc_usable_size(ptr)
    #else
        #define xr_counted_alloc_size(ptr, size) (size)
        #define xr_counted_free_size(ptr) ((size_t)0)
    #endif

    #if defined(XR_PLATFORM_WINDOWS)
        #include <malloc.h>
        #define xr_internal_free_aligned(ptr, alignment) _aligned_free(ptr)
        #define xr_internal_free_size_aligned(ptr, size, alignment) _aligned_free(ptr)
    #else
        #define xr_internal_free_aligned(ptr, alignment) free(ptr)
        #define xr_internal_free_size_aligned(ptr, size, alignment) free(ptr)
    #endif

    static bool xr_valid_aligned_allocation(size_t size, size_t alignment)
    {
        if (alignment == 0 || (alignment & (alignment - 1)) != 0)
        {
            errno = EINVAL;
            return false;
        }
        if (size > size_t(-1) - xr_reserved_tail)
        {
            errno = ENOMEM;
            return false;
        }
        return true;
    }

    static void* xr_internal_malloc_aligned(size_t size, size_t alignment)
    {
        if (!xr_valid_aligned_allocation(size, alignment))
            return nullptr;
        const size_t allocationSize = size + xr_reserved_tail > 0 ? size + xr_reserved_tail : 1;
    #if defined(XR_PLATFORM_WINDOWS)
        return _aligned_malloc(allocationSize, alignment);
    #else
        void* result = nullptr;
        const int error = posix_memalign(&result,
            alignment < sizeof(void*) ? sizeof(void*) : alignment, allocationSize);
        if (error)
        {
            errno = error;
            return nullptr;
        }
        return result;
    #endif
    }

    static void* xr_internal_realloc_aligned(void* ptr, size_t size, size_t alignment)
    {
        if (ptr && size == 0)
        {
            xr_internal_free_aligned(ptr, alignment);
            return nullptr;
        }
        if (!xr_valid_aligned_allocation(size, alignment))
            return nullptr;
        if (!ptr)
            return xr_internal_malloc_aligned(size, alignment);
        const size_t allocationSize = size + xr_reserved_tail > 0 ? size + xr_reserved_tail : 1;
    #if defined(XR_PLATFORM_WINDOWS)
        return _aligned_realloc(ptr, allocationSize, alignment);
    #else
        #if defined(XR_PLATFORM_APPLE) || defined(XR_PLATFORM_LINUX)
            const size_t previousSize = xr_counted_free_size(ptr);
            if (allocationSize <= previousSize && (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0)
                return ptr;
        #endif
        void* result = xr_internal_malloc_aligned(size, alignment);
        if (!result)
            return nullptr;
        #if defined(XR_PLATFORM_APPLE) || defined(XR_PLATFORM_LINUX)
            memcpy(result, ptr, previousSize < allocationSize ? previousSize : allocationSize);
            free(ptr);
        #else
            void* resized = realloc(ptr, allocationSize);
            if (!resized)
            {
                free(result);
                return nullptr;
            }
            memcpy(result, resized, allocationSize);
            free(resized);
        #endif
        return result;
    #endif
    }

    #define xr_internal_malloc(size) malloc(size + xr_reserved_tail)
    #define xr_internal_malloc_nothrow(size) malloc(size + xr_reserved_tail)
    #define xr_internal_malloc_nothrow_aligned(size, alignment) xr_internal_malloc_aligned(size, alignment)
    #define xr_internal_small_alloc(size) malloc(size + xr_reserved_tail)
    #define xr_internal_small_free(ptr) free(ptr)

    #define xr_internal_realloc(ptr, size) realloc(ptr, size + xr_reserved_tail)

    #define xr_internal_free(ptr) free(ptr)
    #define xr_internal_free_size(ptr, size) free(ptr)
    #define xr_counted_alloc_size_aligned(ptr, size) xr_counted_alloc_size(ptr, size)
    #define xr_counted_free_size_aligned(ptr) xr_counted_free_size(ptr)
#else
    #error Please, define explicitly which allocator you want to use
#endif

xrMemory Memory;
// Also used in src\xrCore\xrDebug.cpp to prevent use of g_pStringContainer before it initialized
bool shared_str_initialized = false;

void xrMemory::_initialize()
{
    ZoneScoped;
    g_pStringContainer = xr_new<str_container>();
    shared_str_initialized = true;
    g_pSharedMemoryContainer = xr_new<smem_container>();
}

void xrMemory::_destroy()
{
    ZoneScoped;
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
#elif defined(XR_PLATFORM_HAIKU)
    *_free = *reserved = *committed = 0;
    system_info info;
    if (get_system_info(&info) == B_OK)
    {
        *_free = B_PAGE_SIZE * (uint64)(info.max_pages - info.used_pages);
        *reserved = B_PAGE_SIZE * (uint64)info.cached_pages;
        *committed = B_PAGE_SIZE * (uint64)info.used_pages;
    }
#endif
}

XRCORE_API void log_vminfo()
{
    size_t w_free, w_reserved, w_committed;
    vminfo(&w_free, &w_reserved, &w_committed);
    Msg("* [ %s ]: free[%zu K], reserved[%zu K], committed[%zu K]", SDL_GetPlatform(), w_free / 1024, w_reserved / 1024, w_committed / 1024);
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
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (size_t)ru.ru_maxrss;
#elif defined(XR_PLATFORM_HAIKU)
    system_info info;
    get_system_info(&info);
    return B_PAGE_SIZE * (uint64)info.used_pages;
#else
    return 0;
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
    const auto result = xr_internal_malloc(size);
    xray::memstats::CountAlloc(xr_counted_alloc_size(result, size));
    //TracyAlloc(result, size);
    return result;
}

void* xrMemory::mem_alloc(size_t size, size_t alignment)
{
    const auto result = xr_internal_malloc_aligned(size, alignment);
    if (result)
        xray::memstats::CountAlloc(xr_counted_alloc_size_aligned(result, size));
    return result;
}

void* xrMemory::mem_alloc(size_t size, const std::nothrow_t&) noexcept
{
    const auto result = xr_internal_malloc_nothrow(size);
    xray::memstats::CountAlloc(xr_counted_alloc_size(result, size));
    //TracyAlloc(result, size);
    return result;
}

void* xrMemory::mem_alloc(size_t size, size_t alignment, const std::nothrow_t&) noexcept
{
    const auto result = xr_internal_malloc_nothrow_aligned(size, alignment);
    if (result)
        xray::memstats::CountAlloc(xr_counted_alloc_size_aligned(result, size));
    return result;
}

void* xrMemory::small_alloc(size_t size) noexcept
{
    const auto result = xr_internal_small_alloc(size);
    xray::memstats::CountAlloc(xr_counted_alloc_size(result, size));
    //TracyAllocN(result, size, "small alloc");
    return result;
}

void xrMemory::small_free(void* ptr) noexcept
{
    //TracyFree(ptr);
    if (ptr)
        xray::memstats::CountFree(xr_counted_free_size(ptr));
    xr_internal_small_free(ptr);
}

void* xrMemory::mem_realloc(void* ptr, size_t size)
{
    //TracyFree(ptr);
    if (ptr)
        xray::memstats::CountFree(xr_counted_free_size(ptr));
    const auto result = xr_internal_realloc(ptr, size);
    xray::memstats::CountAlloc(xr_counted_alloc_size(result, size));
    //TracyAllocN(result, size, "realloc");
    return result;
}

void* xrMemory::mem_realloc(void* ptr, size_t size, size_t alignment)
{
    const size_t previousSize = ptr ? xr_counted_free_size_aligned(ptr) : 0;
    const auto result = xr_internal_realloc_aligned(ptr, size, alignment);
    if (ptr && (result || size == 0))
        xray::memstats::CountFree(previousSize);
    if (result)
        xray::memstats::CountAlloc(xr_counted_alloc_size_aligned(result, size));
    return result;
}

void xrMemory::mem_free(void* ptr)
{
    //TracyFree(ptr);
    if (ptr)
        xray::memstats::CountFree(xr_counted_free_size(ptr));
    xr_internal_free(ptr);
}

void xrMemory::mem_free(void* ptr, size_t alignment)
{
    //TracyFree(ptr);
    if (ptr)
        xray::memstats::CountFree(xr_counted_free_size_aligned(ptr));
    xr_internal_free_aligned(ptr, alignment);
}

// xr_strdup
XRCORE_API pstr xr_strdup(pcstr string)
{
#ifdef USE_MIMALLOC
    const auto result = mi_strdup(string);
    xray::memstats::CountAlloc(mi_usable_size(result));
    return result;
#else
    VERIFY(string);
    size_t len = xr_strlen(string) + 1;
    auto memory = static_cast<char*>(xr_malloc(len));
    CopyMemory(memory, string, len);
    return memory;
#endif
}

[[nodiscard]] void* operator new(size_t size)
{
    return Memory.mem_alloc(size);
}

[[nodiscard]] void* operator new[](size_t size)
{
    return Memory.mem_alloc(size);
}

[[nodiscard]] void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return Memory.mem_alloc(size);
}

[[nodiscard]] void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return Memory.mem_alloc(size);
}

[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment)
{
    return Memory.mem_alloc(size, static_cast<size_t>(alignment));
}

[[nodiscard]] void* operator new[](size_t size, std::align_val_t alignment)
{
    return Memory.mem_alloc(size, static_cast<size_t>(alignment));
}

[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return Memory.mem_alloc(size, static_cast<size_t>(alignment), std::nothrow);
}

[[nodiscard]] void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return Memory.mem_alloc(size, static_cast<size_t>(alignment), std::nothrow);
}

void operator delete(void* ptr) noexcept
{
    Memory.mem_free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    Memory.mem_free(ptr);
}

void operator delete(void* ptr, std::align_val_t alignment) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}

void operator delete[](void* ptr, std::align_val_t alignment) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}

void operator delete(void* ptr, size_t) noexcept
{
    Memory.mem_free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept
{
    Memory.mem_free(ptr);
}

void operator delete(void* ptr, size_t, std::align_val_t alignment) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}

void operator delete[](void* ptr, size_t, std::align_val_t alignment) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}

void operator delete(void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}

void operator delete[](void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}

XRCORE_API void* xr_malloc(size_t size)
{
    return Memory.mem_alloc(size);
}

XRCORE_API void* xr_realloc(void* ptr, size_t size)
{
    return Memory.mem_realloc(ptr, size);
}
