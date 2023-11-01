#pragma once

#include "xr_types.h"

#include <new>

class XRCORE_API xrMemory
{
public:
    xrMemory() = default;
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
[[nodiscard]] void* operator new[](size_t size);
[[nodiscard]] void* operator new(size_t size, const std::nothrow_t&) noexcept;
[[nodiscard]] void* operator new[](size_t size, const std::nothrow_t&) noexcept;
[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment);
[[nodiscard]] void* operator new[](size_t size, std::align_val_t alignment);
[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
[[nodiscard]] void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;

void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, std::align_val_t alignment) noexcept;
void operator delete[](void* ptr, std::align_val_t alignment) noexcept;
void operator delete(void* ptr, size_t) noexcept;
void operator delete[](void* ptr, size_t) noexcept;
void operator delete(void* ptr, size_t, std::align_val_t alignment) noexcept;
void operator delete[](void* ptr, size_t, std::align_val_t alignment) noexcept;

// generic "C"-like allocations/deallocations

template<typename T>
T* xr_alloc(size_t count)
{
    return static_cast<T*>(Memory.mem_alloc(count * sizeof(T)));
}

template<typename T>
void xr_free(T*& ptr) noexcept
{
    if (ptr)
    {
        Memory.mem_free((void*)ptr);
        ptr = nullptr;
    }
}

template<bool, typename T>
struct xr_special_free
{
    IC void operator()(T*& ptr)
    {
        auto real_ptr = dynamic_cast<void*>(ptr);
        ptr->~T();
        xr_free(real_ptr);
    }
};

template<typename T>
struct xr_special_free<false, T>
{
    IC void operator()(T*& ptr)
    {
        ptr->~T();
        xr_free(ptr);
    }
};

template<typename T, typename... Args>
T* xr_new(Args&&... args)
{
    auto ptr = static_cast<T*>(Memory.mem_alloc(sizeof(T)));
    return new (ptr) T(std::forward<Args>(args)...);
}

template<typename T>
void xr_delete(T*& ptr) noexcept
{
    if (ptr)
    {
        xr_special_free<std::is_polymorphic_v<T>, T>()(ptr);
    }
    ptr = nullptr;
}

template<typename T>
void xr_delete(T* const& ptr) noexcept
{
    auto hacked_ptr = const_cast<T*&>(ptr);
    if (hacked_ptr)
    {
        xr_special_free<std::is_polymorphic_v<T>, T>()(hacked_ptr);
    }
    hacked_ptr = nullptr;
}

XRCORE_API void* xr_malloc(size_t size);
XRCORE_API void* xr_realloc(void* ptr, size_t size);

XRCORE_API pstr xr_strdup(pcstr string);

XRCORE_API void log_vminfo();
