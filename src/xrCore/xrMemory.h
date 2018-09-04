#pragma once

#include "_types.h"

#include "tbb/tbb_allocator.h"
#include "tbb/scalable_allocator.h"

/*
Можно заключить - прокси перехватывает не всегда и/или не всё используемые функции.
И в очень малом количестве случаев это приводит к странностям при работе с памятью.
Поэтому всё-же стоит переопределять для большинства случаев операторы new и delete.
А для остального мы будем полагать (и надеяться), что прокси справится без проблем.
*/
#if defined(WINDOWS) // I have not idea how it works on Windows, but Linux build fails with error 'multiple declaration of __TBB_malloc_proxy_helper_object'
#include "tbb/tbbmalloc_proxy.h"
#endif

class XRCORE_API xrMemory
{
    // Additional 16 bytes of memory almost like in original xr_aligned_offset_malloc
    // But for DEBUG we don't need this if we want to find memory problems
#ifdef DEBUG
    size_t reserved = 0;
#else
    size_t reserved = 16;
#endif

public:
    xrMemory();
    void _initialize();
    void _destroy();

    u32 stat_calls;

public:
    size_t mem_usage();
    void   mem_compact();
    inline void* mem_alloc             (size_t size) { stat_calls++; return scalable_malloc (     size + reserved); };
    inline void* mem_realloc(void* ptr, size_t size) { stat_calls++; return scalable_realloc(ptr, size + reserved); };
    inline void  mem_free   (void* ptr)              { stat_calls++;        scalable_free   (ptr);                  };
};

extern XRCORE_API xrMemory Memory;

#undef ZeroMemory
#undef CopyMemory
#undef FillMemory
#define ZeroMemory(a, b) memset(a, 0, b)
#define CopyMemory(a, b, c) memcpy(a, b, c)
#define FillMemory(a, b, c) memset(a, c, b)

/*
Начиная со стандарта C++11 нет необходимости объявлять все формы операторов new и delete.
*/

inline void* operator new(size_t size)
{
    return Memory.mem_alloc(size);
}

inline void operator delete(void* ptr) noexcept
{
    Memory.mem_free(ptr);
}

template <class T>
inline void xr_delete(T*& ptr) noexcept
{
    delete ptr;
    ptr = nullptr;
}

// generic "C"-like allocations/deallocations
template <class T>
inline T* xr_alloc(size_t count)
{
    return (T*)Memory.mem_alloc(count * sizeof(T));
}
template <class T>
inline void xr_free(T*& ptr) noexcept
{
    if (ptr)
    {
        Memory.mem_free((void*)ptr);
        ptr = nullptr;
    }
}
inline void* xr_malloc            (size_t size) { return Memory.mem_alloc       (size); }
inline void* xr_realloc(void* ptr, size_t size) { return Memory.mem_realloc(ptr, size); }

XRCORE_API pstr xr_strdup(pcstr string);

XRCORE_API void log_vminfo();
