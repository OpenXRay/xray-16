#pragma once

#include "Include/xrRender/RenderFactory.h"

namespace xray::render::vulkan
{
class VulkanGameDevice;

// Device-scoped factory. The remaining engine contracts are filled in by
// their own milestones before this factory is installed in GEnv.
class VulkanRenderFactory final : public IRenderFactory
{
public:
    explicit VulkanRenderFactory(VulkanGameDevice& device) : device_(device) {}
#define VK_FACTORY_METHODS(Class) \
    I##Class* Create##Class() override; \
    void Destroy##Class(I##Class* object) override;
#ifndef _EDITOR
    VK_FACTORY_METHODS(UISequenceVideoItem)
    VK_FACTORY_METHODS(UIShader)
    VK_FACTORY_METHODS(StatGraphRender)
#ifdef DEBUG
    VK_FACTORY_METHODS(ObjectSpaceRender)
#endif
    VK_FACTORY_METHODS(WallMarkArray)
    VK_FACTORY_METHODS(EnvironmentRender)
    VK_FACTORY_METHODS(EnvDescriptorRender)
    VK_FACTORY_METHODS(RainRender)
    VK_FACTORY_METHODS(LensFlareRender)
    VK_FACTORY_METHODS(ImGuiRender)
    VK_FACTORY_METHODS(ThunderboltRender)
    VK_FACTORY_METHODS(ThunderboltDescRender)
    VK_FACTORY_METHODS(FlareRender)
#endif
    VK_FACTORY_METHODS(FontRender)
#undef VK_FACTORY_METHODS

private:
    VulkanGameDevice& device_;
};
}
