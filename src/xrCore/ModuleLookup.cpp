#include "stdafx.h"

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD)
#include <dlfcn.h>
#else
#include <SDL3/SDL_loadso.h>
#endif

#include "ModuleLookup.hpp"

namespace XRay
{
ModuleHandle::ModuleHandle(const bool dontUnload) : handle(nullptr), dontUnload(dontUnload) {}

ModuleHandle::ModuleHandle(pcstr moduleName, bool dontUnload /*= false*/) : handle(nullptr), dontUnload(dontUnload)
{
    this->Open(moduleName);
}

ModuleHandle::~ModuleHandle()
{
    Close();
}

void* ModuleHandle::Open(pcstr moduleName)
{
    ZoneScoped;

    if (IsLoaded())
        Close();

    Log("Loading module:", moduleName);

    xr_string buf(moduleName);
#ifdef XR_PLATFORM_WINDOWS
    buf += ".dll";
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD)
    buf += ".so";
#elif defined(XR_PLATFORM_APPLE)
    buf += ".dylib";
#else
#error add your platform-specific extension here
#endif

    pcstr error = nullptr;
#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD)
    // For platforms that use rpath we have to call dlopen() from our own module
    handle = dlopen(buf.c_str(), RTLD_NOW);
    if (!handle)
        error = dlerror();
#else
    handle = SDL_LoadObject(buf.c_str());
    if (!handle)
        error = SDL_GetError();
#endif

    if (!handle)
    {
        Log("! Failed to load module:", moduleName);
        if (error)
            Log("!", error);
    }

    return handle;
}

void ModuleHandle::Close()
{
    ZoneScoped;

    if (dontUnload || !handle)
        return;

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD)
    dlclose(handle);
#else
    SDL_UnloadObject(handle);
#endif
    handle = nullptr;
}

bool ModuleHandle::IsLoaded() const
{
    return handle != nullptr;
}

void* ModuleHandle::operator()() const
{
    return handle;
}

void* ModuleHandle::GetProcAddress(pcstr procName) const
{
    pcstr error = nullptr;
#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD)
    const auto proc = dlsym(handle, procName);
    if (!proc)
        error = dlerror();
#else
    const auto proc = SDL_LoadFunction(handle, procName);
    if (!proc)
        error = SDL_GetError();
#endif

    if (!proc)
    {
        Log("! Failed to load function from module:", procName);
        if (error)
            Log("!", error);
    }

    return proc;
}
} // namespace XRay
