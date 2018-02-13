#pragma once

#include "_types.h"

#include "tbb/tbb_allocator.h"
#include "tbb/tbbmalloc_proxy.h"

class XRCORE_API xrMemory
{
public:
    xrMemory();
    void _initialize();
    void _destroy();

    u32 stat_calls;

public:
    size_t mem_usage();
    void mem_compact();
    void* mem_alloc(size_t size);
    void* mem_realloc(void* p, const size_t size);
    void mem_free(void* p);
};

extern XRCORE_API xrMemory Memory;

#undef ZeroMemory
#undef CopyMemory
#undef FillMemory
#define ZeroMemory(a, b) memset(a, 0, b)
#define CopyMemory(a, b, c) memcpy(a, b, c)
#define FillMemory(a, b, c) memset(a, c, b)

#define xr_delete(x)\
{\
    delete (x);\
    (x) = nullptr;\
}

// generic "C"-like allocations/deallocations
template <class T>
T* xr_alloc(const size_t count)
{ return (T*)Memory.mem_alloc(count * sizeof(T)); }


template <class T>
void xr_free(T*& P) noexcept
{
    if (P)
    {
        Memory.mem_free((void*)P);
        P = nullptr;
    }
}
inline void* xr_malloc(const size_t size) { return Memory.mem_alloc(size); }
inline void* xr_realloc(void* P, const size_t size) { return Memory.mem_realloc(P, size); }

XRCORE_API pstr xr_strdup(pcstr string);

XRCORE_API void log_vminfo();
