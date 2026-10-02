#include "xrEngine/stdafx.h"
#include "VulkanRenderFactory.h"
#include "VulkanGameDevice.h"
#include "VulkanUIShader.h"
#include "VulkanFontRender.h"
#include "VulkanStatGraphRender.h"
#include "VulkanWallMarkArray.h"
#include "VulkanImGuiRender.h"
#include "VulkanUISequenceVideoItem.h"

namespace xray::render::vulkan
{
IUIShader* VulkanRenderFactory::CreateUIShader()
{
    return new VulkanUIShader(device_.textures(), device_.ui_pass());
}
void VulkanRenderFactory::DestroyUIShader(IUIShader* object)
{
    delete object;
}
IFontRender* VulkanRenderFactory::CreateFontRender()
{
    return new VulkanFontRender(device_);
}
void VulkanRenderFactory::DestroyFontRender(IFontRender* object)
{
    delete object;
}
IImGuiRender* VulkanRenderFactory::CreateImGuiRender()
{
    return new VulkanImGuiRender(device_);
}
void VulkanRenderFactory::DestroyImGuiRender(IImGuiRender* object)
{
    delete object;
}
IUISequenceVideoItem* VulkanRenderFactory::CreateUISequenceVideoItem()
{
    return new VulkanUISequenceVideoItem(device_);
}
void VulkanRenderFactory::DestroyUISequenceVideoItem(IUISequenceVideoItem* object)
{
    delete object;
}

IStatGraphRender* VulkanRenderFactory::CreateStatGraphRender()
{
    return new VulkanStatGraphRender(device_);
}
void VulkanRenderFactory::DestroyStatGraphRender(IStatGraphRender* object)
{
    delete object;
}

IWallMarkArray* VulkanRenderFactory::CreateWallMarkArray()
{
    return new VulkanWallMarkArray(device_);
}
void VulkanRenderFactory::DestroyWallMarkArray(IWallMarkArray* object)
{
    delete object;
}

// These contracts belong to the subsequent environment and debug milestones.
#define VK_PENDING_FACTORY(Class) \
    I##Class* VulkanRenderFactory::Create##Class() { return nullptr; } \
    void VulkanRenderFactory::Destroy##Class(I##Class* object) \
    { R_ASSERT2(!object, "Vulkan factory object has not been implemented"); }
#ifndef _EDITOR
#ifdef DEBUG
VK_PENDING_FACTORY(ObjectSpaceRender)
#endif
VK_PENDING_FACTORY(EnvironmentRender)
VK_PENDING_FACTORY(EnvDescriptorRender)
VK_PENDING_FACTORY(RainRender)
VK_PENDING_FACTORY(LensFlareRender)
VK_PENDING_FACTORY(ThunderboltRender)
VK_PENDING_FACTORY(ThunderboltDescRender)
VK_PENDING_FACTORY(FlareRender)
#endif
#undef VK_PENDING_FACTORY
}
