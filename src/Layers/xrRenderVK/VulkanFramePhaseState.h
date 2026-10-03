#pragma once

namespace xray::render::vulkan
{
// Validates the engine's optional world phase and its menu/UI callback order
// without tying frame recording to an OpenGL context or GPU command state.
class VulkanFramePhaseState
{
public:
    bool begin()
    {
        if (active_) return false;
        active_ = true;
        calculated_ = false;
        rendered_ = false;
        menu_rendered_ = false;
        return true;
    }

    bool calculate()
    {
        if (!active_ || calculated_ || rendered_ || menu_rendered_) return false;
        calculated_ = true;
        return true;
    }

    bool render()
    {
        if (!active_ || !calculated_ || rendered_) return false;
        rendered_ = true;
        return true;
    }

    bool render_menu()
    {
        if (!active_ || menu_rendered_ || (calculated_ && !rendered_)) return false;
        menu_rendered_ = true;
        return true;
    }

    bool can_end() const
    {
        return active_ && (!calculated_ || rendered_);
    }

    bool end()
    {
        if (!can_end()) return false;
        reset();
        return true;
    }

    void reset()
    {
        active_ = false;
        calculated_ = false;
        rendered_ = false;
        menu_rendered_ = false;
    }

    bool active() const { return active_; }
    bool world_calculated() const { return calculated_; }
    bool world_rendered() const { return rendered_; }

private:
    bool active_{};
    bool calculated_{};
    bool rendered_{};
    bool menu_rendered_{};
};
}
