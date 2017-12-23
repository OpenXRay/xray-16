#pragma once
#include <vector>
#include "xrCore/Memory/xalloc.h"

#define DEF_VECTOR(N, T)\
    typedef xr_vector<T> N;\
    typedef N::iterator N##_it;

#define DEFINE_VECTOR(T, N, I)\
    typedef xr_vector<T> N;\
    typedef N::iterator I;

//template <typename T, typename allocator = xalloc<T>>
//using xr_vector = class std::vector<T, allocator>;

// vector
template <typename T, typename allocator = xalloc<T>>
class xr_vector : public std::vector<T, allocator>
{
    using inherited = std::vector<T, allocator>;

public:
    using allocator_type = allocator;

    xr_vector() : inherited() {}
    xr_vector(size_t _count) : inherited(_count) {}
    xr_vector(size_t _count, const T& _value) : inherited(_count, _value) {}
    IC void clear_and_free()    { clear(); }
    IC void clear_not_free()    { clear(); }
    IC void clear_and_reserve() { clear(); }
};
