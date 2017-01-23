#pragma once

#include "xrCore.h"

namespace XRay {
XRCORE_API HMODULE LoadLibrary(const char *libraryFileName, bool log = true);
XRCORE_API void UnloadLibrary(HMODULE libraryHandle);
XRCORE_API void *GetProcAddress(HMODULE libraryHandle, const char *procName);
}
