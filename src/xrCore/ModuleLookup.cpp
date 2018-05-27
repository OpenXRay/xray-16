#include "stdafx.h"

#include "ModuleLookup.hpp"

namespace XRay
{
ModuleHandle::ModuleHandle(const bool dontUnload) : handle(nullptr), dontUnload(dontUnload) {}

ModuleHandle::ModuleHandle(pcstr moduleName, bool dontUnload /*= false*/) : handle(nullptr), dontUnload(dontUnload)
{
    open(moduleName);
}

ModuleHandle::~ModuleHandle()
{
    close();
}

void* ModuleHandle::open(pcstr moduleName)
{
    if (exist())
        close();
    
    Log("Loading DLL:", moduleName);

#ifdef WINDOWS
    handle = LoadLibraryA(moduleName);
#elif defined(LINUX)
    handle = dlopen(name, RTLD_LAZY);
#endif
    if (handle == nullptr)
    {
#ifdef WINDOWS
        Msg("! Failed to load DLL: 0x%d", GetLastError());
#elif defined(LINUX)
        Msg("! Failed to load DLL: 0x%d", dlerror());
#endif
    }

    return handle;
}

void ModuleHandle::close()
{
    if (dontUnload)
        return;

    bool closed = false;

#ifdef WINDOWS
    closed = FreeLibrary(static_cast<HMODULE>(handle)) != 0;
#else
    closed = dlclose(handle) == 0;
#endif

    if (closed == false)
    {
#ifdef WINDOWS
        Msg("! Failed to close DLL: 0x%d", GetLastError());
#elif LINUX
        Msg("! Failed to close DLL: 0x%d", dlerror());
#endif
    }

    handle = nullptr;
}

bool ModuleHandle::exist() const
{
    return handle != nullptr;
}

void* ModuleHandle::operator()() const
{
    return handle;
}

void* ModuleHandle::getProcAddress(pcstr procName) const
{
    void* proc = nullptr;

#ifdef WINDOWS
    proc = ::GetProcAddress(static_cast<HMODULE>(handle), procName);
#elif defined(LINUX)
    proc = dlsym(handle, procedure);
#endif

    if (proc == nullptr)
    {
#ifdef WINDOWS
        Msg("! Failed to load procedure [%s] from DLL: 0x%d", procName, GetLastError());
#elif LINUX
        Msg("! Failed to load procedure [%s] from DLL: 0x%d", procName, dlerror());
#endif
    }

    return proc;
}
} // namespace XRay
