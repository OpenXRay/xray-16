// xrPhysics.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"
#include "xrPhysics.h"
#include <ode/memory.h>

#ifdef _MANAGED
#pragma managed(push, off)
#endif

static void* ode_alloc(size_t size) { return xr_malloc(size); }
static void* ode_realloc(void* ptr, size_t oldsize, size_t newsize) { return xr_realloc(ptr, newsize); }
static void ode_free(void* ptr, size_t size) { return xr_free(ptr); }

class ode_memory_initializer final
{
public:
    ode_memory_initializer()
    {
        dSetAllocHandler(ode_alloc);
        dSetReallocHandler(ode_realloc);
        dSetFreeHandler(ode_free);
    }
} static s_ode_memory_initializer;

#ifdef _MANAGED
#pragma managed(pop)
#endif
