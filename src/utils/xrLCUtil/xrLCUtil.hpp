#pragma once

#include "xrCore/xrCore.h"

#ifdef XR_PLATFORM_WINDOWS
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")
#endif

#ifdef XRAY_STATIC_BUILD
#define XRLCUTIL_API
#else
#   ifdef XRLCUTIL_EXPORTS
#       define XRLCUTIL_API XR_EXPORT
#   else
#       define XRLCUTIL_API XR_IMPORT
#   endif
#endif

XRLCUTIL_API std::string make_time(u32 sec);
