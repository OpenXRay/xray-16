#pragma once

#include "_types.h"

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
    void* mem_realloc(void* ptr, size_t size);
    void mem_free(void* ptr);
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

inline void operator delete(void* ptr, size_t) noexcept
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
