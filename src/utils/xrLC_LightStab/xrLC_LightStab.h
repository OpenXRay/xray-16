#pragma once

#include "xrCore/Platform.hpp"

#ifdef XRLC_LIGHT_STAB_EXPORTS
#	define XRLC_LIGHT_STUB_API XR_EXPORT
#else
#	define XRLC_LIGHT_STUB_API XR_IMPORT
#endif


#	define XRLC_LIGHT_API XR_IMPORT