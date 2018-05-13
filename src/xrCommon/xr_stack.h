#pragma once
#include <stack>
#include "xr_vector.h"

template <typename T, class container = xr_vector<T>>
using xr_stack = std::stack<T, container>;

#define DEFINE_STACK(T, N) using N = xr_stack<T>;
