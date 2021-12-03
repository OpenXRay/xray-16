#pragma once

#include "Common/Common.hpp"

#include "xrCore/xrCore.h"

#define ECORE_API

#include "xrEngine/Engine.h"

#include "xrServerEntities/smart_cast.h"

#include "xrCDB/xrCDB.h"
#include "xrSound/Sound.h"
#include "xrEngine/GameMtlLib.h"
#include "xrCore/_std_extensions.h"

#include "xrPhysics.h"

// XXX: WHY IS THIS HERE
#if defined(DEBUG) && defined(XR_PLATFORM_WINDOWS)
#include <d3d9types.h>
#endif
