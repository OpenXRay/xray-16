#include "src/Layers/xrRenderVK/VulkanFramePhaseState.h"

#include <cassert>

using namespace xray::render::vulkan;

int main()
{
    VulkanFramePhaseState frame;
    assert(!frame.active() && !frame.can_end());

    // A menu-only frame has UI callbacks but does not require a world pass.
    assert(frame.begin());
    assert(!frame.begin());
    assert(frame.render_menu());
    assert(!frame.render_menu());
    assert(!frame.calculate());
    assert(frame.can_end() && frame.end());
    assert(!frame.active());

    // In a gameplay frame the world is calculated and rendered before menu
    // callbacks; End is rejected if the world was only calculated.
    assert(frame.begin());
    assert(frame.calculate());
    assert(!frame.can_end());
    assert(!frame.render_menu());
    assert(frame.render());
    assert(frame.world_calculated() && frame.world_rendered());
    assert(frame.render_menu());
    assert(!frame.render_menu());
    assert(frame.can_end() && frame.end());
    assert(!frame.render());
}
