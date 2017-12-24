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
    doug_lea_allocator(void* arena, u32 arena_size, pcstr arena_id);
    ~doug_lea_allocator();
    void* malloc_impl(u32 size);
    void* realloc_impl(void* pointer, u32 new_size);
    void free_impl(void*& pointer);
    u32 get_allocated_size() const;
    pcstr get_arena_id() const { return m_arena_id; }
    template <typename T>
    void free_impl(T*& pointer)
    {
        free_impl(reinterpret_cast<void*&>(pointer));
    }

    template <typename T>
    T* alloc_impl(u32 const count)
    {
        return (T*)malloc_impl(count * sizeof(T));
    }

private:
    pcstr m_arena_id;
    void* m_dl_arena;
};

extern doug_lea_allocator common;

template <class T, doug_lea_allocator& _impl = common>
class doug_lea_alloc
{
    constexpr static doug_lea_allocator& impl = _impl;

public:
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = T * ;
    using const_pointer = const T*;
    using reference = T & ;
    using const_reference = const T&;
    using value_type = T;

    template <class _Other>
    struct rebind
    {
        using other = doug_lea_alloc<_Other>;
    };

    doug_lea_alloc() {}
    doug_lea_alloc(const doug_lea_alloc<T>&) {}

    template <class _Other>
    doug_lea_alloc(const doug_lea_alloc<_Other>&) {}

    template <class _Other>
    doug_lea_alloc<T>& operator=(const doug_lea_alloc<_Other>&)
    {
        return *this;
    }

    pointer address(reference _Val) const { return (&_Val); }
    const_pointer address(const_reference _Val) const { return (&_Val); }

    pointer allocate(size_type n, const void* /*p*/ = nullptr) const
    {
        return static_cast<T*>(impl.malloc_impl(sizeof(T) * (u32)n));
    }

    void deallocate(pointer p, size_type) const { impl.free_impl((void*&)p); }
    void deallocate(void* p, size_type n) const { impl.free_impl(p); }
    char* __charalloc(size_type n) { return (char*)allocate(n); }
    void construct(pointer p, const T& _Val) { new(p) T(_Val); }
    void destroy(pointer p) { p->~T(); }

    size_type max_size() const
    {
        constexpr size_type _Count = static_cast<size_type>(-1) / sizeof(T);
        return 0 < _Count ? _Count : 1;
    }
};

template <class _Ty, class _Other>
bool operator==(const doug_lea_alloc<_Ty>&, const doug_lea_alloc<_Other>&)
{
    return (true);
}

template <class _Ty, class _Other>
bool operator!=(const doug_lea_alloc<_Ty>&, const doug_lea_alloc<_Other>&)
{
    return (false);
}

template <doug_lea_allocator& _impl = common>
struct doug_lea_allocator_wrapper
{
    constexpr static doug_lea_allocator& impl = _impl;

    template <typename T>
    struct helper
    {
        using result = doug_lea_alloc<T, _impl>;
    };

    static void* alloc(const u32& n) { return impl.malloc_impl((u32)n); }

    template <typename T>
    static void dealloc(T*& p)
    {
        impl.free_impl((void*&)p);
    }
};
#endif // USE_DOUG_LEA_ALLOCATOR
