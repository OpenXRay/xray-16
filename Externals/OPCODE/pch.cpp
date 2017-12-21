#include "pch.hpp"

#ifdef USE_ARENA_ALLOCATOR
static const u32 s_arena_size = (128 + 16) * 1024 * 1024;
static char s_fake_array[s_arena_size];
doug_lea_allocator g_collision_allocator(s_fake_array, s_arena_size, "opcode");
#endif // #ifdef USE_ARENA_ALLOCATOR
