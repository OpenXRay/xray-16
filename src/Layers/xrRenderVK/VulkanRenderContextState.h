#pragma once

namespace xray::render::vulkan
{
// Vulkan has one device and one frame-recording context. Engine helper-context
// scopes therefore resolve to that same primary context; no GL context is
// created or switched.
class VulkanRenderContextState
{
public:
    static constexpr int NoContext = -1;
    static constexpr int PrimaryContext = 0;
    static constexpr int HelperContext = 1;

    void device_created() noexcept { current_ = PrimaryContext; }
    void device_destroyed() noexcept { current_ = NoContext; }

    bool make_current(int context) noexcept
    {
        switch (context)
        {
        case NoContext:
            current_ = NoContext;
            return true;
        case PrimaryContext:
        case HelperContext:
            current_ = PrimaryContext;
            return true;
        default:
            return false;
        }
    }

    int current() const noexcept { return current_; }

private:
    int current_{NoContext};
};
}
