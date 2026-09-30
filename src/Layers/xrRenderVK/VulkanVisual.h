#pragma once

#include "Include/xrRender/RenderVisual.h"
#include "xrEngine/vis_common.h"
#include "LevelModels.h"

namespace xray::render::vulkan
{
class GpuLevel;

// The engine-facing identity and bounds of a level visual. GPU meshes remain
// owned by GpuLevel, so a level unload invalidates all visual pointers.
class VulkanVisual final : public IRenderVisual
{
public:
    VulkanVisual(GpuLevel& owner, uint32_t index, const LevelVisual& visual);
    vis_data& getVisData() override { return visibility_; }
    u32 getType() const override { return type_; }
    IRenderVisual* getSubModel(u8 index) override;
    uint32_t index() const { return index_; }
    const GpuLevel& owner() const { return owner_; }
#ifdef DEBUG
    shared_str getDebugName() override { return name_; }
#endif

private:
    GpuLevel& owner_;
    uint32_t index_;
    u32 type_;
    vis_data visibility_;
#ifdef DEBUG
    shared_str name_;
#endif
};
}
