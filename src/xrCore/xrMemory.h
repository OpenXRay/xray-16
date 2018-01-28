#pragma once

#include "Memory/memory_allocator_options.h"
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

// delete

template <bool _is_pm, typename T>
struct xr_special_free
{
    IC void operator()(T*& ptr)
    {
        void* _real_ptr = dynamic_cast<void*>(ptr);
        ptr->~T();
        Memory.mem_free(_real_ptr);
    }
};

template <typename T>
struct xr_special_free<false, T>
{
    IC void operator()(T*& ptr)
    {
        ptr->~T();
        Memory.mem_free(ptr);
    }
};

template <class T>
IC void xr_delete(T*& ptr)
{
    if (ptr)
    {
        xr_special_free<std::is_polymorphic<T>::value, T>()(ptr);
        ptr = NULL;
    }
}
template <class T>
IC void xr_delete(T* const& ptr)
{
    if (ptr)
    {
        xr_special_free<std::is_polymorphic<T>::value, T>(ptr);
        const_cast<T*&>(ptr) = NULL;
    }
}

// generic "C"-like allocations/deallocations
template <class T>
T* xr_alloc(const size_t count)
{ return (T*)Memory.mem_alloc(count * sizeof(T)); }


template <class T>
void xr_free(T*& P) throw()
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

// Global new/delete override
#ifndef NO_XRNEW
#if !defined(BUILDING_XRMISC_LIB) && defined(_MSC_VER)
#pragma comment(lib, "xrMisc") // Attempt to force the TU to include our version.
#endif
// XXX: Implementations of operator new/delete are in xrMisc/xrMemory.cpp, since they need
// to be in a static link library.
void* operator new(const size_t size);
void operator delete(void* p);
void* operator new[](const size_t size);
void operator delete[](void* p);
#endif

XRCORE_API void log_vminfo();
