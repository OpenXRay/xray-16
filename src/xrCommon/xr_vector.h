#pragma once
#include <vector>
#include "xr_allocator.h"

template <typename T, typename allocator = xr_allocator<T>>
using xr_vector = std::vector<T, allocator>;

#define DEF_VECTOR(N, T)\
    using N = xr_vector<T>;\
    using N##_it = N::iterator;

#define DEFINE_VECTOR(T, N, I)\
    using N = xr_vector<T>;\
    using I = N::iterator;
