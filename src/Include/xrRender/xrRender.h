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
namespace fg
{
XRRENDER_R4_API RendererModule* GetFrameGraphRendererModule();
}
} // namespace xray::render
