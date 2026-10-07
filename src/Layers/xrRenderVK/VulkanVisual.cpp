#include "xrEngine/stdafx.h"
#include "VulkanVisual.h"
#include "GpuLevel.h"

namespace xray::render::vulkan
{
VulkanVisual::VulkanVisual(GpuLevel& owner, uint32_t index, const LevelVisual& visual)
    : owner_(owner), index_(index), type_(visual.type)
{
    visibility_.clear();
    const auto& b = visual.bounds;
    visibility_.box.set(b[0], b[1], b[2], b[3], b[4], b[5]);
    visibility_.sphere.P.set(b[6], b[7], b[8]);
    visibility_.sphere.R = b[9];
#ifdef DEBUG
    string64 name;
    xr_sprintf(name, "vulkan_level_visual_%u", index);
    name_ = name;
#endif
}

IRenderVisual* VulkanVisual::getSubModel(u8 child)
{
    const LevelVisual* visual = owner_.visual_node(index_);
    return visual && child < visual->children.size() ?
        owner_.get_visual(visual->children[child]) : nullptr;
}
}
