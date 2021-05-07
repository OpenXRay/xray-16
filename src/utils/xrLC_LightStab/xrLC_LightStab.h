#pragma once

#include "Common/Platform.hpp"
#include "xrCore/_types.h"

#ifdef XRAY_STATIC_BUILD
#define XRLC_LIGHT_STUB_API
#else
#   ifdef XRLC_LIGHT_STAB_EXPORTS
#      define XRLC_LIGHT_STUB_API XR_EXPORT
#   else
#      define XRLC_LIGHT_STUB_API XR_IMPORT
#   endif
#endif

#include "utils/xrLC_Light/xrLC_Light.h"
