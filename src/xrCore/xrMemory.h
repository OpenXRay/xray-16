#pragma once

#include "_types.h"

#include <new>

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
    void* mem_alloc(size_t size, const std::nothrow_t&) noexcept;
    void* mem_alloc(size_t size, size_t alignment, const std::nothrow_t&) noexcept;
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

// Global C++ new/delete overrides.
[[nodiscard]] inline void* operator new(size_t size)
{
    return Memory.mem_alloc(size);
}

[[nodiscard]] inline void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return Memory.mem_alloc(size);
}

[[nodiscard]] inline void* operator new(size_t size, std::align_val_t alignment)
{
    return Memory.mem_alloc(size, static_cast<size_t>(alignment));
}

[[nodiscard]] inline void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return Memory.mem_alloc(size, static_cast<size_t>(alignment));
}

inline void operator delete(void* ptr) noexcept
{
    Memory.mem_free(ptr);
}

inline void operator delete(void* ptr, std::align_val_t alignment) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}

inline void operator delete(void* ptr, size_t) noexcept
{
    Memory.mem_free(ptr);
}

inline void operator delete(void* ptr, size_t, std::align_val_t alignment) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}

template <typename T, typename... Args>
inline T* xr_new(Args&&... args)
{
    auto ptr = static_cast<T*>(Memory.mem_alloc(sizeof(T)));
    return new (ptr) T(std::forward<Args>(args)...);
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
