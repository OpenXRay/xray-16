#pragma once
#include <vector>
#include "xrCore/Memory/XRayAllocator.hpp"

#define DEF_VECTOR(N, T)\
    typedef xr_vector<T> N;\
    typedef N::iterator N##_it;

#define DEFINE_VECTOR(T, N, I)\
    typedef xr_vector<T> N;\
    typedef N::iterator I;

template <typename T, typename allocator = XRay::xray_allocator<T>>
using xr_vector = class std::vector<T, allocator>;
