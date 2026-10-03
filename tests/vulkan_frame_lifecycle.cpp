#include "src/Layers/xrRenderVK/FrameContext.h"

#include <cassert>

using namespace xray::render::vulkan;

int main()
{
    bool surface_lost = false;
    FrameStatus status = FrameStatus::Presented;
    assert(!mark_surface_lost(VK_SUCCESS, surface_lost, status));
    assert(!surface_lost && status == FrameStatus::Presented);

    assert(mark_surface_lost(VK_ERROR_SURFACE_LOST_KHR, surface_lost, status));
    assert(surface_lost && status == FrameStatus::RecreateRequired);

    status = FrameStatus::Presented;
    assert(mark_surface_lost(VK_ERROR_SURFACE_LOST_KHR, surface_lost, status));
    assert(surface_lost && status == FrameStatus::RecreateRequired);
}
