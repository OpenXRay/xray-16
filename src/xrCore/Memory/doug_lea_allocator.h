////////////////////////////////////////////////////////////////////////////
// Created : 14.08.2009
// Author : Armen Abroyan
// Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef USE_DOUG_LEA_ALLOCATOR
class XRCORE_API doug_lea_allocator
{
public:
    doug_lea_allocator(void* arena, size_t arena_size, pcstr arena_id);
    ~doug_lea_allocator();
    void* malloc_impl(size_t size);
    void* realloc_impl(void* pointer, size_t new_size);
    void free_impl(void* pointer);
    size_t get_allocated_size() const;
    pcstr get_arena_id() const { return m_arena_id; }
    template <typename T>
    void free_impl(T*& pointer)
    {
        free_impl(reinterpret_cast<void*&>(pointer));
    }

    template <typename T>
    T* alloc_impl(const size_t count)
    {
        return (T*)malloc_impl(count * sizeof(T));
    }

private:
    pcstr m_arena_id;
    void* m_dl_arena;
};

extern doug_lea_allocator g_common_doug_lea_allocator;

template <class T, doug_lea_allocator& _impl = g_common_doug_lea_allocator>
class doug_lea_alloc
{
    constexpr static doug_lea_allocator& impl = _impl;

public:
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using value_type = T;

    template <class Other>
    struct rebind
    {
        using other = doug_lea_alloc<Other>;
    };

    doug_lea_alloc() = default;
    doug_lea_alloc(const doug_lea_alloc<T, _impl>&) = default;

    template <class Other>
    doug_lea_alloc(const doug_lea_alloc<Other>&) {}

    template <class Other>
    doug_lea_alloc<T>& operator=(const doug_lea_alloc<Other>&)
    {
        return *this;
    }

    pointer address(reference ref) const { return &ref; }
    const_pointer address(const_reference ref) const { return &ref; }

    pointer allocate(size_type n, const void* /*p*/ = nullptr) const
    {
        return static_cast<pointer>(impl.malloc_impl(sizeof(T) * n));
    }

    void deallocate(pointer p, size_type /*n*/) const { impl.free_impl((void*&)p); }
    void deallocate(void* p, size_type /*n*/) const { impl.free_impl(p); }
    void construct(pointer p, const T& ref) { new(p) T(ref); }
    void destroy(pointer p) { p->~T(); }

    size_type max_size() const
    {
        constexpr auto count = std::numeric_limits<size_type>::max() / sizeof(T);
        return 0 < count ? count : 1;
    }
};

template <class T, class Other>
bool operator==(const doug_lea_alloc<T>&, const doug_lea_alloc<Other>&)
{
    return true;
}

template <class T, class Other>
bool operator!=(const doug_lea_alloc<T>&, const doug_lea_alloc<Other>&)
{
    return false;
}

template <doug_lea_allocator& _impl = g_common_doug_lea_allocator>
struct doug_lea_allocator_wrapper
{
    constexpr static doug_lea_allocator& impl = _impl;

    template <typename T>
    struct helper
    {
        using result = doug_lea_alloc<T, _impl>;
    };

    static void* alloc(const size_t& n) { return impl.malloc_impl(n); }

    template <typename T>
    static void dealloc(T*& p)
    {
        impl.free_impl((void*&)p);
    }
};
#endif // USE_DOUG_LEA_ALLOCATOR
