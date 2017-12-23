#include "stdafx.h"
#include "RenderAllocator.hpp"

#ifdef USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
#ifdef USE_ARENA_ALLOCATOR
static const u32 s_arena_size = 8 * 1024 * 1024;
static char s_fake_array[s_arena_size];
doug_lea_allocator g_render_allocator(s_fake_array, s_arena_size, "render");
#else
doug_lea_allocator g_render_allocator(nullptr, 0, "render");
#endif
#endif
