#include "stdafx.h"

#include "ModuleLookup.hpp"

namespace XRay
{
Module::Module() : handle(nullptr) {}

Module::Module(pcstr moduleName, bool log /*= true*/)
{
    open(moduleName, log);
}

Module::~Module()
{
    close();
}

void* Module::open(pcstr moduleName, bool log /*= true*/)
{
    if (exist())
        close();

    if (log)
        Log("Loading DLL:", moduleName);

    handle = ::LoadLibrary(moduleName);
    return handle;
}

void Module::close()
{
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

HMODULE LoadLibrary(const char* libraryFileName, bool log)
{
    if (log)
        Log("Loading DLL:", libraryFileName);
    return ::LoadLibraryA(libraryFileName);
}

void UnloadLibrary(HMODULE libraryHandle) { FreeLibrary(libraryHandle); }
void* GetProcAddress(HMODULE libraryHandle, const char* procName) { return ::GetProcAddress(libraryHandle, procName); }
}
