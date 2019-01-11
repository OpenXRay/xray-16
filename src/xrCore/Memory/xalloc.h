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

    static pointer allocate(const size_type n, const void* p = nullptr) { return xr_alloc<T>(n); }
    static void deallocate(pointer p, const size_type /*n*/) { xr_free(p); }
    static void deallocate(void* p, const size_type /*n*/) { xr_free(p); }

    template <class U, class... Args>
    static void construct(U* ptr, Args&&... args)
    {
        new (ptr) U(std::forward<Args>(args)...);
    }

    template <class U>
    static void destroy(U* p)
    {
        p->~U();
    }

    static constexpr size_type max_size()
    {
        constexpr auto count = std::numeric_limits<size_type>::max() / sizeof(T);
        return count > 0 ? count : 1;
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
