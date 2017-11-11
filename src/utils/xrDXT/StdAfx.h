#pragma once

#include "Common/Platform.hpp"
#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ENGINE_API
#define XR_EPROPS_API
#define ECORE_API

#define USE_NVTT

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)\
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#endif // defined(MAKEFOURCC)

#pragma comment(lib, "xrCore.lib")
#pragma warning(disable : 4995)
