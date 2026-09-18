#pragma once

#include "Common/Platform.hpp"

#ifndef XRCORE_API
#  define XRCORE_API XR_IMPORT
#  define TRACY_IMPORTS
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_string.h"
#include "xrCore/_std_extensions.h"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/xrsharedmem.h"

#include "xrCommon/xr_map.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrCore/Animation/SkeletonMotions.hpp"
