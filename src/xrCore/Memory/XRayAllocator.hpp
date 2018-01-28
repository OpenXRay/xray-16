#pragma once
#include "memory_allocator_options.h"
#include "xalloc.h"

namespace XRay
{
    template<typename T>
    using xray_allocator = xalloc<T>;
    using xray_allocator_wrapper = xr_allocator;
} // namespace XRay
