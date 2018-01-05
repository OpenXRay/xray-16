#pragma once
#include <list>
#include "xrCore/Memory/XRayAllocator.hpp"

template <typename T, typename allocator = XRay::xray_allocator<T>>
using xr_list = std::list<T, allocator>;

#define DEF_LIST(N, T)\
    using N = xr_list<T>;\
    using N##_it = N::iterator;

#define DEFINE_LIST(T, N, I)\
    using N = xr_list<T>;\
    using I = N::iterator;
