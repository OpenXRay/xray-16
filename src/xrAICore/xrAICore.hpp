#pragma once

#include "xrCore/Platform.hpp"

#ifdef XRAICORE_EXPORTS
#define XRAICORE_API XR_EXPORT
#else
#define XRAICORE_API XR_IMPORT
#endif
