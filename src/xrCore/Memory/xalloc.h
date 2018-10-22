#pragma once
#include "xrCore/xrMemory.h"

template <typename T>
class xalloc
{
public:
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using value_type = T;

    template <class Other>
    struct rebind
    {
        using other = xalloc<Other>;
    };

    pointer address(reference ref) const { return &ref; }
    const_pointer address(const_reference ref) const { return &ref; }

    xalloc() = default;
    xalloc(const xalloc<T>&) = default;

    template <class Other>
    xalloc(const xalloc<Other>&)
    {
    }

    template <class Other>
    xalloc& operator=(const xalloc<Other>&)
    {
        return *this;
    }

    pointer allocate(const size_type n, const void* p = nullptr) const { return xr_alloc<T>(n); }
    void deallocate(pointer p, const size_type /*n*/) const { xr_free(p); }
    void deallocate(void* p, const size_type /*n*/) const { xr_free(p); }

    template <class U, class... Args>
    void construct(U* ptr, Args&&... args)
    {
        new (ptr) U(std::forward<Args>(args)...);
    }
    template <class U>
    void destroy(U* p)
    {
        p->~U();
    }
    size_type max_size() const
    {
        constexpr auto count = std::numeric_limits<size_type>::max() / sizeof(T);
        return 0 < count ? count : 1;
    }
};

struct xr_allocator
{
    template <typename T>
    struct helper
    {
        using result = xalloc<T>;
    };

    static void* alloc(const size_t n) { return xr_malloc(n); }

    template <typename T>
    static void dealloc(T*& p)
    {
        xr_free(p);
    }
};

template <class T, class Other>
bool operator==(const xalloc<T>&, const xalloc<Other>&)
{
    return true;
}

template <class T, class Other>
bool operator!=(const xalloc<T>&, const xalloc<Other>&)
{
    return false;
}
