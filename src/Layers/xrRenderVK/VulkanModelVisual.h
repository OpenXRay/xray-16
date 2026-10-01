#pragma once

#include "GpuModel.h"
#include "Include/xrRender/RenderVisual.h"
#include "xrEngine/vis_common.h"

namespace xray::render::vulkan
{
// An independent, renderer-owned static OGF instance. GPU resources are
// released before its owning Vulkan device and material factory.
class VulkanModelVisual final : public IRenderVisual
{
public:
    explicit VulkanModelVisual(const VisualRecord& record)
    {
        visibility_.clear();
        const auto& b = record.bounds;
        visibility_.box.set(b[0], b[1], b[2], b[3], b[4], b[5]);
        visibility_.sphere.P.set(b[6], b[7], b[8]);
        visibility_.sphere.R = b[9];
    }

    vis_data& getVisData() override { return visibility_; }
    u32 getType() const override { return 0; }
    GpuModel& gpu() { return gpu_; }
#ifdef DEBUG
    shared_str getDebugName() override { return "vulkan_static_ogf"; }
#endif

private:
    vis_data visibility_;
    GpuModel gpu_;
};
}
