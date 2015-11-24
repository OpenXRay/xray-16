#include "stdafx.h"

#include "ModuleLookup.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace XRay {
HMODULE LoadLibrary(const char *libraryFileName, bool log)
{
    if (log)
        Log("Loading DLL:", libraryFileName);
    return ::LoadLibrary(libraryFileName);
}

void UnloadLibrary(HMODULE libraryHandle)
{
    FreeLibrary(libraryHandle);
}

void *GetProcAddress(HMODULE libraryHandle, const char *procName)
{
    return ::GetProcAddress(libraryHandle, procName);
}

}
