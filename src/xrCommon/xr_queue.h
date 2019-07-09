#pragma once
#include <queue>
#include "xrCommon/xr_deque.h"

template <typename T>
using xr_queue = std::queue<T, xr_deque<T>>;
