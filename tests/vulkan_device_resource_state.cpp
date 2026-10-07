#include "src/Layers/xrRenderVK/VulkanDeviceResourceState.h"

#include <cassert>

using namespace xray::render::vulkan;

int main()
{
    VulkanDeviceResourceState state;
    assert(!state.can_create_factory_objects() && !state.can_load_level());
    assert(!state.setup_states());
    assert(state.device_created());
    assert(!state.can_create_factory_objects() && !state.can_load_level());
    assert(state.setup_states());
    assert(state.device_resources_created());
    assert(state.can_create_factory_objects() && state.can_load_level());

    // UI/font factory objects remain available while a level is replaced.
    assert(state.device_resources_destroyed());
    assert(!state.can_create_factory_objects() && !state.can_load_level());
    assert(state.setup_states());
    assert(state.device_resources_created());
    assert(state.device_resources_destroyed());
    assert(state.device_destroyed());
    assert(!state.can_create_factory_objects() && !state.can_load_level());
}
