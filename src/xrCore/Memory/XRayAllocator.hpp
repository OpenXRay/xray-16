#pragma once
#include "memory_allocator_options.h"
#include "xalloc.h"
#ifdef USE_DOUG_LEA_ALLOCATOR
#include "doug_lea_allocator.h"
#endif

namespace XRay
{
    template<typename T>
    using xray_allocator = xalloc<T>;
    using xray_allocator_wrapper = xr_allocator;
} // namespace XRay
