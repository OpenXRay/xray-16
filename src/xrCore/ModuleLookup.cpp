#include "stdafx.h"

#include <SDL_loadso.h>

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
    if (IsLoaded())
        Close();

    Log("Loading module:", moduleName);

    xr_string buf(moduleName);

#ifdef WINDOWS
    buf += ".dll";
#elif defined(LINUX) || defined(FREEBSD)
    buf += ".so";
#else
#error add your platform-specific extension here
#endif

    handle = SDL_LoadObject(buf.c_str());

    if (!handle)
    {
        Log("! Failed to load module:", moduleName);
        Log("!", SDL_GetError());
    }

    return handle;
}

void ModuleHandle::Close()
{
    if (dontUnload)
        return;

    SDL_UnloadObject(handle);
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
    const auto proc = SDL_LoadFunction(handle, procName);

    if (!proc)
    {
        Log("! Failed to load function from module:", procName);
        Log("!", SDL_GetError());
    }

    return proc;
}
} // namespace XRay
