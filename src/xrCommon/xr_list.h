#pragma once
#include <list>
#include "xrCore/xrMemory.h"

template <typename T, typename allocator = xr_allocator<T>>
using xr_list = std::list<T, allocator>;

#define DEF_LIST(N, T)\
    using N = xr_list<T>;\
    using N##_it = N::iterator;

#define DEFINE_LIST(T, N, I)\
    using N = xr_list<T>;\
    using I = N::iterator;
