#pragma once

#include "xr_types.h"
#include "_rect.h"
#include "_plane.h"
#include "_vector2.h"
#include "_vector3d.h"
#include "_color.h"
#include "_std_extensions.h"
#include "xrMemory.h"
#include "xrCommon/xr_array.h"
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_map.h"
#include "xrCommon/xr_set.h"
#include "xrCommon/xr_stack.h"
#include "xrCommon/xr_list.h"
#include "xrCommon/xr_deque.h"
#include "xrstring.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_unordered_map.h"
#include "xrDebug_macros.h" // only for pragma todo. Remove once handled.

#pragma todo("tamlin: This header includes pretty much every std collection there are. Compiler-hog! FIX!")

#include "xrCommon/predicates.h"

// tamlin: TODO (low priority, for a rainy day): Rename these macros from DEFINE_* to DECLARE_*
// Xottab_DUTY: TODO: or maybe use Im-Dex variant (Get rid of this DEFINE macroses)

// STL extensions
#include "FixedVector.h"
#include "buffer_vector.h"
