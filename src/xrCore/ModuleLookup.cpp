#include "stdafx.h"

#include "ModuleLookup.hpp"

namespace XRay
{
Module::Module(const bool dontUnload) : handle(nullptr), dontUnload(dontUnload) {}

Module::Module(pcstr moduleName, bool dontUnload /*= false*/) : handle(nullptr), dontUnload(dontUnload)
{
    open(moduleName);
}

Module::~Module()
{
    close();
}

void* Module::open(pcstr moduleName)
{
    if (exist())
        close();
    
    Log("Loading DLL:", moduleName);

    handle = ::LoadLibrary(moduleName);

    if (handle == nullptr)
        Msg("Failed to load DLL: %d", GetLastError());

    return handle;
}

void Module::close()
{
    if (dontUnload)
        return;

    FreeLibrary(static_cast<HMODULE>(handle));
    handle = nullptr;
}

bool Module::exist() const
{
    return handle != nullptr;
}

void* Module::operator()() const
{
    return handle;
}

void* Module::getProcAddress(pcstr procName) const
{
    return ::GetProcAddress(static_cast<HMODULE>(handle), procName);
}
}
