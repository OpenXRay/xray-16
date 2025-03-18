#pragma once

#include "r_constants.h"

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/dx11r_constants_cache.h"
#elif defined(USE_OGL)
#include "Layers/xrRenderGL/glr_constants_cache.h"
#else
#   error No graphics API selected or enabled!
#endif
