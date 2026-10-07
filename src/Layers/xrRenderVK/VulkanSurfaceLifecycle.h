#pragma once

#include <cstdint>

namespace xray::render::vulkan
{
enum class SurfaceState { Lost, NeedReset, Normal };
// Keep Android app events and drawable availability separate: the surface can
// reappear a few events after foreground notification, including at zero size.
class VulkanSurfaceLifecycle
{
public:
    void reset() { suspended_ = false; pending_ = false; replace_surface_ = false; extent_ = {}; }
    bool change_activity(bool active)
    {
        if (suspended_ == !active) return false;
        suspended_ = !active;
        if (active) { pending_ = true; replace_surface_ = true; }
        return true;
    }
    void request_reset() { pending_ = true; }
    void finish_reset(uint32_t width, uint32_t height)
    {
        if (!width || !height) return;
        extent_ = {width, height};
        pending_ = replace_surface_ = false;
    }
    SurfaceState state(int width, int height, bool device_lost, bool renderer_reset) const
    {
        if (device_lost || suspended_ || width <= 0 || height <= 0) return SurfaceState::Lost;
        if (pending_ || renderer_reset || extent_.width != static_cast<uint32_t>(width) ||
            extent_.height != static_cast<uint32_t>(height)) return SurfaceState::NeedReset;
        return SurfaceState::Normal;
    }
    bool replace_surface() const { return replace_surface_; }

private:
    struct Extent { uint32_t width{}, height{}; } extent_;
    bool suspended_{};
    bool pending_{};
    bool replace_surface_{};
};
}
