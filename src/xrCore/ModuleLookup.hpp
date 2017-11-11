#pragma once

#include "xrCore.h"

namespace XRay
{
class XRCORE_API Module
{
    void* handle;

public:
    Module();
    Module(pcstr moduleName, bool log = true);
    ~Module();

    void* open(pcstr moduleName, bool log = true);;
    void close();

    bool exist() const;

    void* operator()() const;

    void* getProcAddress(pcstr procName) const;
};

XRCORE_API HMODULE LoadLibrary(const char* libraryFileName, bool log = true);
XRCORE_API void UnloadLibrary(HMODULE libraryHandle);
XRCORE_API void* GetProcAddress(HMODULE libraryHandle, const char* procName);
}
