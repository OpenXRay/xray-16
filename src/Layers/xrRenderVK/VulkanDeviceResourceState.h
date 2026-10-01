#pragma once

namespace xray::render::vulkan
{
// Engine device creation calls SetupStates and OnDeviceCreate before loading
// a level. Keep that order explicit so font and UI shader objects can be
// created from device resources without depending on level lifetime.
class VulkanDeviceResourceState
{
public:
    bool device_created()
    {
        if (stage_ != Stage::Empty) return false;
        stage_ = Stage::Device;
        return true;
    }

    bool setup_states()
    {
        if (stage_ == Stage::States || stage_ == Stage::Resources) return true;
        if (stage_ != Stage::Device) return false;
        stage_ = Stage::States;
        return true;
    }

    bool device_resources_created()
    {
        if (stage_ == Stage::Resources) return true;
        if (stage_ != Stage::States) return false;
        stage_ = Stage::Resources;
        return true;
    }

    bool device_resources_destroyed()
    {
        if (stage_ == Stage::States) return true;
        if (stage_ != Stage::Resources) return false;
        stage_ = Stage::States;
        return true;
    }

    bool device_destroyed()
    {
        if (stage_ == Stage::Empty) return true;
        if (stage_ == Stage::Resources) return false;
        stage_ = Stage::Empty;
        return true;
    }

    bool can_create_factory_objects() const { return stage_ == Stage::Resources; }
    bool can_load_level() const { return stage_ == Stage::Resources; }

private:
    enum class Stage
    {
        Empty,
        Device,
        States,
        Resources
    };

    Stage stage_{Stage::Empty};
};
}
