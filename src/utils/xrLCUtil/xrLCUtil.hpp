#pragma once
#include "xrCore/xrCore.h"
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")

#ifdef XRLCUTIL_EXPORTS
#define XRLCUTIL_API XR_EXPORT
#else
#define XRLCUTIL_API XR_IMPORT
#endif

XRLCUTIL_API std::string make_time(u32 sec);
