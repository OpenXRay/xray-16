#include "stdafx.h"
#include "RenderContext/RenderDevice.h"

namespace xray::render::ng {

// Global modern render device instance
// This is used by the ImGui renderer and other modern rendering systems
// It should be initialized during renderer startup
RenderDevice* g_ModernRenderDevice = nullptr;

} // namespace xray::render::ng