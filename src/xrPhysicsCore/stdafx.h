#pragma once
#include "Common/Common.hpp"
#include "xrCore/xrCore.h"

// xr_types.h replaces FLT_MAX with an expression; Jolt's headers require the
// standard floating-point macro during constant initialization.
#undef FLT_MAX
#define FLT_MAX 3.402823466e+38F
