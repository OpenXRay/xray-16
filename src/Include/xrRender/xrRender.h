#pragma once

#include "xrEngine/EngineAPI.h"

#ifdef XRAY_STATIC_BUILD
#    define XRRENDER_R4_API
#else
#    ifdef XRRENDER_R4_EXPORTS
#        define XRRENDER_R4_API XR_EXPORT
#    else
#        define XRRENDER_R4_API XR_IMPORT
#    endif
#endif

namespace xray::render
{
#ifdef XR_PLATFORM_WINDOWS
namespace fg
{
XRRENDER_R4_API RendererModule* GetFrameGraphRendererModule();
}
#endif
} // namespace xray::render
