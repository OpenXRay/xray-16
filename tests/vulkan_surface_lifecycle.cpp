#include "src/Layers/xrRenderVK/VulkanSurfaceLifecycle.h"
#include <cassert>

using namespace xray::render::vulkan;

int main()
{
    VulkanSurfaceLifecycle lifecycle;
    lifecycle.finish_reset(1280, 720);
    assert(lifecycle.state(1280, 720, false, false) == SurfaceState::Normal);
    for (int cycle = 0; cycle < 5; ++cycle)
    {
        assert(lifecycle.change_activity(false));
        assert(!lifecycle.change_activity(false));
        assert(lifecycle.state(1280, 720, false, false) == SurfaceState::Lost);
        assert(lifecycle.change_activity(true));
        assert(lifecycle.replace_surface());
        assert(lifecycle.state(0, 0, false, false) == SurfaceState::Lost);
        lifecycle.finish_reset(0, 0);
        assert(lifecycle.replace_surface());
        assert(lifecycle.state(1280, 720, false, false) == SurfaceState::NeedReset);
        lifecycle.finish_reset(1280, 720);
        assert(!lifecycle.replace_surface());
        assert(lifecycle.state(1280, 720, false, false) == SurfaceState::Normal);
    }
    assert(lifecycle.state(720, 1280, false, false) == SurfaceState::NeedReset);
    lifecycle.request_reset();
    lifecycle.finish_reset(720, 1280);
    assert(lifecycle.state(720, 1280, false, false) == SurfaceState::Normal);
    assert(lifecycle.state(720, 1280, true, false) == SurfaceState::Lost);
    assert(lifecycle.state(720, 1280, false, true) == SurfaceState::NeedReset);
}
