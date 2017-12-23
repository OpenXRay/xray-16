#pragma once
#include "xrCore/Memory/doug_lea_allocator.h"

#ifdef USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
extern doug_lea_allocator g_render_allocator;

template<class T>
using render_alloc = doug_lea_alloc<T, g_render_allocator>;
using render_allocator = doug_lea_allocator_wrapper<g_render_allocator>;
#else
template<class T>
using render_alloc = xalloc<T>;
using render_allocator = xr_allocator;
#endif
