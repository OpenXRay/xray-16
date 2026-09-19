#pragma once

#include "Common/Common.hpp"

#ifdef XRAY_STATIC_BUILD
#define XRANIMATION_API
#elif defined(XRANIMATION_EXPORTS)
#define XRANIMATION_API XR_EXPORT
#else
#define XRANIMATION_API XR_IMPORT
#endif
