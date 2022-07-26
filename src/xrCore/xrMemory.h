#pragma once

#include "xr_types.h"

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

public:
    static constexpr size_t SMALL_SIZE_MAX = 128 * sizeof(void*);

public:
    void* mem_alloc(size_t size);
    void* mem_alloc(size_t size, size_t alignment);
    void* mem_alloc(size_t size, const std::nothrow_t&) noexcept;
    void* mem_alloc(size_t size, size_t alignment, const std::nothrow_t&) noexcept;
    void* mem_realloc(void* ptr, size_t size);
    void* mem_realloc(void* ptr, size_t size, size_t alignment);
    void mem_free(void* ptr);
    void mem_free(void* ptr, size_t alignment);

    void* small_alloc(size_t size) noexcept;
    void  small_free (void* ptr) noexcept;
};

extern XRCORE_API xrMemory Memory;

class small_buffer final
{
    void* m_ptr;
    bool m_small;
    
public:
    small_buffer(size_t size)
        : m_ptr(size <= xrMemory::SMALL_SIZE_MAX ? Memory.small_alloc(size) : Memory.mem_alloc(size)),
          m_small(size <= xrMemory::SMALL_SIZE_MAX) {}

    ~small_buffer()
    {
        if (m_small)
            Memory.small_free(m_ptr);
        else
            Memory.mem_free(m_ptr);
    }

    // Movable
    small_buffer(small_buffer&&) = default;
    small_buffer& operator=(small_buffer&&) = default;

    // Noncopyable
    small_buffer(const small_buffer&) = delete;
    small_buffer& operator=(const small_buffer&) = delete;

    [[nodiscard]]
    void* get() const { return m_ptr; }
};

#undef ZeroMemory
#undef CopyMemory
#undef FillMemory
#define ZeroMemory(dst, size) memset(dst, 0, size)
#define CopyMemory(dst, src, size) memcpy(dst, src, size)
#define FillMemory(dst, size, val) memset(dst, val, size)

// Global C++ new/delete overrides.
[[nodiscard]] void* operator new(size_t size);
[[nodiscard]] void* operator new(size_t size, const std::nothrow_t&) noexcept;
[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment);
[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
void operator delete(void* ptr) noexcept;
void operator delete(void* ptr, std::align_val_t alignment) noexcept;
void operator delete(void* ptr, size_t) noexcept;
void operator delete(void* ptr, size_t, std::align_val_t alignment) noexcept;

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
