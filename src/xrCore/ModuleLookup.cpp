#include "stdafx.h"

#include "ModuleLookup.hpp"
#ifdef LINUX
#include <dlfcn.h>
#endif

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

    Log("Loading DLL:", moduleName);

#ifdef WINDOWS
    handle = LoadLibraryA(moduleName);
#elif defined(LINUX)
    std::string soName = std::string(moduleName) + ".so";
    handle = dlopen(soName.c_str(), RTLD_LAZY);
#endif
    if (handle == nullptr)
    {
#ifdef WINDOWS
        Msg("! Failed to load DLL: 0x%d", GetLastError());
#elif defined(LINUX)
        Msg("! Failed to load shared library %s: %s", soName.c_str(), dlerror());
#endif
    }

    return handle;
}

void ModuleHandle::Close()
{
    if (dontUnload)
        return;

    bool closed = false;

#ifdef WINDOWS
    closed = FreeLibrary(static_cast<HMODULE>(handle)) != 0;
#elif defined(LINUX)
    closed = dlclose(handle) == 0;
#endif

    if (closed == false)
    {
#ifdef WINDOWS
        Msg("! Failed to close DLL: 0x%d", GetLastError());
#elif defined(LINUX)
        Msg("! Failed to close shared library: %s", dlerror());
#endif
    }

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
    void* proc = nullptr;

#ifdef WINDOWS
    proc = ::GetProcAddress(static_cast<HMODULE>(handle), procName);
#elif defined(LINUX)
    proc = dlsym(handle, procName);
#endif

    if (proc == nullptr)
    {
#ifdef WINDOWS
        Msg("! Failed to load procedure [%s] from DLL: 0x%d", procName, GetLastError());
#elif defined(LINUX)
        Msg("! Failed to load procedure [%s] from shared library: %s", procName, dlerror());
#endif
    }

    return proc;
}
} // namespace XRay
