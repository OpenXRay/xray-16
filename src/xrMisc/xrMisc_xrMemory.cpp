#include "pch.hpp"
#include "xrCore/xrMemory.h"
#include "xrCore/Memory/XRayAllocator.hpp"

#if !defined(BUILDING_XRMISC_LIB) || defined(XRCORE_EXPORTS)
#error BUILDING_XRMISC_LIB MUST be defined when building xrMisc
#error XRCORE_EXPORTS MUST NOT be defined when building xrMisc
#endif

#ifndef NO_XRNEW
void* operator new(const size_t size) { return Memory.mem_alloc(size); }
void* operator new[](const size_t size) { return Memory.mem_alloc(size); }

void operator delete(void* p) throw() { Memory.mem_free(p); }
void operator delete[](void* p) throw() { Memory.mem_free(p); }
#endif
