#pragma once

#include "_types.h"
#include "Memory/xr_memory_router.hpp"

class XRCORE_API xrMemory
{
public:
    xrMemory();
    void _initialize();
    void _destroy();

    u32 stat_calls;

public:
    size_t mem_usage();
    void   mem_compact();

    void* mem_alloc(size_t size);
    void* mem_alloc(size_t size, size_t alignment);
    void* mem_alloc(size_t size, const std::nothrow_t&);
    void* mem_alloc(size_t size, size_t alignment, const std::nothrow_t&);
    void* mem_realloc(void* ptr, size_t size);
    void* mem_realloc(void* ptr, size_t size, size_t alignment);
    void mem_free(void* ptr);
    void mem_free(void* ptr, size_t alignment);
};

extern XRCORE_API xrMemory Memory;

#undef ZeroMemory
#undef CopyMemory
#undef FillMemory
#define ZeroMemory(dst, size) memset(dst, 0, size)
#define CopyMemory(dst, src, size) memcpy(dst, src, size)
#define FillMemory(dst, size, val) memset(dst, val, size)

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
