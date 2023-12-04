#pragma once

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrCore/_std_extensions.h"
#include "xrCore/xr_resource.h"

#include "xrCDB/xrCDB.h"

#include "Sound.h"

#if __has_include(<phonon.h>) && defined(XR_PLATFORM_WINDOWS)
#   include <phonon.h>
#   define USE_PHONON
#endif
