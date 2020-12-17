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

static void setup_ode_memory_handlers()
{
    dSetAllocHandler(ode_alloc);
    dSetReallocHandler(ode_realloc);
    dSetFreeHandler(ode_free);
}

#if defined(XR_PLATFORM_WINDOWS)
BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        setup_ode_memory_handlers();
        break;

    default: break;
    }
    return TRUE;
}
#elif defined(XR_PLATFORM_LINUX)
__attribute__((constructor)) static void load(int /*argc*/, char** /*argv*/, char** /*envp*/)
{
    setup_ode_memory_handlers();
}
#else
#error Add your platform here
#endif //XR_PLATFORM_WINDOWS

#ifdef _MANAGED
#pragma managed(pop)
#endif
