#include "Common/Common.hpp"

class RGLRendererModule;

#ifndef XR_PLATFORM_SWITCH
extern "C"
{
#endif
XR_EXPORT RendererModule* GetRendererModule();
XR_EXPORT void xrRenderGL_GlobalInit();
#ifndef XR_PLATFORM_SWITCH
}
#endif