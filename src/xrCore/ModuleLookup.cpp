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

    handle = ::LoadLibrary(moduleName);

    if (handle == nullptr)
        Msg("! Failed to load DLL: %d", GetLastError());

    return handle;
}

void ModuleHandle::close()
{
    if (dontUnload)
        return;

    FreeLibrary(static_cast<HMODULE>(handle));
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
    return GetProcAddress(static_cast<HMODULE>(handle), procName);
}
}
