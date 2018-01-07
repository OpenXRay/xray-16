#include "pch.hpp"
#include "xrCore/xrMemory.h"
#include "xrCore/Memory/XRayAllocator.hpp"

#if !defined(BUILDING_XRMISC_LIB) || defined(XRCORE_EXPORTS)
#error BUILDING_XRMISC_LIB MUST be defined when building xrMisc
#error XRCORE_EXPORTS MUST NOT be defined when building xrMisc
#endif

#ifndef NO_XRNEW
#ifdef DEBUG_MEMORY_NAME
void* operator new(size_t size) { return Memory.mem_alloc(size, "C++ NEW"); }
void* operator new[](size_t size) { return Memory.mem_alloc(size, "C++ NEW[]"); }
#else
void* operator new(size_t size) { return Memory.mem_alloc(size); }
void* operator new[](size_t size) { return Memory.mem_alloc(size); }
#endif

void operator delete(void* p) throw() { Memory.mem_free(p); }
void operator delete[](void* p) throw() { Memory.mem_free(p); }
#endif

#ifdef USE_DOUG_LEA_ALLOCATOR
#ifdef USE_ARENA_ALLOCATOR
constexpr static const u32 s_arena_size = 256 * 1024 * 1024;
static char s_fake_array[s_arena_size];
doug_lea_allocator g_common_doug_lea_allocator(s_fake_array, s_arena_size, "common");
#else
doug_lea_allocator g_common_doug_lea_allocator(nullptr, 0, "common");
#endif
#endif // USE_DOUG_LEA_ALLOCATOR
