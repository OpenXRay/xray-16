#pragma once
#include "xrCore/xrMemory.h"

template <class T>
class xalloc
{
public:
    typedef size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

public:
    template<class _Other>
    struct rebind
    {
        typedef xalloc<_Other> other;
    };

public:
    pointer address(reference _Val) const { return &_Val; }
    const_pointer address(const_reference _Val) const { return &_Val; }
    xalloc() {}
    xalloc(const xalloc<T>&) {}
    template<class _Other>
    xalloc(const xalloc<_Other>&) {}
    template<class _Other>
    xalloc<T>& operator=(const xalloc<_Other>&) { return *this; }
    pointer allocate(size_type n, const void* p = nullptr) const { return xr_alloc<T>((u32)n); }
    char* _charalloc(size_type n) { return (char*)allocate(n); }
    void deallocate(pointer p, size_type n) const { xr_free(p); }
    void deallocate(void* p, size_type n) const { xr_free(p); }
    void construct(pointer p, const T& _Val) { std::_Construct(p, _Val); }
    void destroy(pointer p) { std::_Destroy(p); }    
    size_type max_size() const
    {
        size_type _Count = (size_type)(-1)/sizeof(T);
        return 0<_Count ? _Count : 1;
    }
};

struct xr_allocator
{
    template <typename T>
    struct helper
    {
        typedef xalloc<T> result;
    };

    static void* alloc(const u32& n) { return xr_malloc((u32)n); }
    template <typename T>
    static void dealloc(T*& p) { xr_free(p); }
};

template <class _Ty, class _Other>
inline bool operator==(const xalloc<_Ty>&, const xalloc<_Other>&) { return true; }

template <class _Ty, class _Other>
inline bool operator!=(const xalloc<_Ty>&, const xalloc<_Other>&) { return false; }
