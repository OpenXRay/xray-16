// xrMisc/xrMemory.cpp
#ifndef BUILDING_XRMISC_LIB
#error BUILDING_XRMISC_LIB MUST be defined when building xrMisc
#endif

#include "xrCore/xrMemory.h"

#ifdef XRCORE_EXPORTS
#error XRCORE_EXPORTS MUST NOT be defined when building xrMisc
#endif

#ifdef DEBUG_MEMORY_NAME

#if !(defined(__BORLANDC__) || defined(NO_XRNEW))
void* operator new(size_t size) { return Memory.mem_alloc(size, "C++ NEW"); }
void* operator new[](size_t size) { return Memory.mem_alloc(size, "C++ NEW[]"); }
#endif

#else

#if !(defined(__BORLANDC__) || defined(NO_XRNEW))
void* operator new(size_t size) { return Memory.mem_alloc(size); }
void* operator new[](size_t size) { return Memory.mem_alloc(size); }
#endif

#endif

void operator delete(void* p) throw() { Memory.mem_free(p); }
void operator delete[](void* p) throw() { Memory.mem_free(p); }
