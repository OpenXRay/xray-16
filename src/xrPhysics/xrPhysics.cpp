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

#ifdef LINUX
__attribute__((constructor))
#endif
static void load(int argc, char** argv, char** envp)
{
    dSetAllocHandler(ode_alloc);
    dSetReallocHandler(ode_realloc);
    dSetFreeHandler(ode_free);
}

#if defined(WINDOWS)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    lpReserved;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        load(0, nullptr, nullptr);

        break;
    case DLL_PROCESS_DETACH: break;
    }
    return TRUE;
}
#endif //WINDOWS

#ifdef _MANAGED
#pragma managed(pop)
#endif
