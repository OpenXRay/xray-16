#pragma once

#include "xrEngine/EngineAPI.h"

#ifdef XRAY_STATIC_BUILD
#    define XRRENDER_R4_API
#    define XRRENDER_GL_API
#else
#    ifdef XRRENDER_R4_EXPORTS
#        define XRRENDER_R4_API XR_EXPORT
#    else
#        define XRRENDER_R4_API XR_IMPORT
#    endif
#    ifdef XRRENDER_GL_EXPORTS
#        define XRRENDER_GL_API XR_EXPORT
#    else
#        define XRRENDER_GL_API XR_IMPORT
#    endif
#endif

namespace xray::render
{
#ifdef XR_PLATFORM_WINDOWS
namespace render_r4
{
XRRENDER_R4_API RendererModule* GetRendererModule();
}
#endif
namespace render_gl
{
XRRENDER_GL_API RendererModule* GetRendererModule();
}
} // namespace xray::render
