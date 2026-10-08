#include "xrEngine/stdafx.h"
#include "VulkanRenderFactory.h"
#include "VulkanGameDevice.h"
#include "VulkanUIShader.h"
#include "VulkanFontRender.h"
#include "VulkanStatGraphRender.h"
#include "VulkanWallMarkArray.h"
#include "VulkanEnvDescriptorRender.h"
#include "VulkanEnvironmentRender.h"
#include "VulkanRainRender.h"
#include "VulkanLensFlareRender.h"
#include "VulkanThunderboltRender.h"
#include "VulkanDebugRender.h"
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

#ifndef _EDITOR
IEnvDescriptorRender* VulkanRenderFactory::CreateEnvDescriptorRender()
{
    return new VulkanEnvDescriptorRender(device_.textures());
}
void VulkanRenderFactory::DestroyEnvDescriptorRender(IEnvDescriptorRender* object)
{
    delete object;
}
IEnvironmentRender* VulkanRenderFactory::CreateEnvironmentRender()
{
    return new VulkanEnvironmentRender(device_);
}
void VulkanRenderFactory::DestroyEnvironmentRender(IEnvironmentRender* object)
{
    delete object;
}
IRainRender* VulkanRenderFactory::CreateRainRender()
{
    return new VulkanRainRender(device_);
}
void VulkanRenderFactory::DestroyRainRender(IRainRender* object)
{
    delete object;
}
IFlareRender* VulkanRenderFactory::CreateFlareRender()
{
    return new VulkanFlareRender(device_);
}
void VulkanRenderFactory::DestroyFlareRender(IFlareRender* object)
{
    delete object;
}
ILensFlareRender* VulkanRenderFactory::CreateLensFlareRender()
{
    return new VulkanLensFlareRender(device_);
}
void VulkanRenderFactory::DestroyLensFlareRender(ILensFlareRender* object)
{
    delete object;
}
IThunderboltDescRender* VulkanRenderFactory::CreateThunderboltDescRender()
{
    return new VulkanThunderboltDescRender(device_);
}
void VulkanRenderFactory::DestroyThunderboltDescRender(IThunderboltDescRender* object)
{
    delete object;
}
IThunderboltRender* VulkanRenderFactory::CreateThunderboltRender()
{
    return new VulkanThunderboltRender(device_);
}
void VulkanRenderFactory::DestroyThunderboltRender(IThunderboltRender* object)
{
    delete object;
}
#endif

#ifndef _EDITOR
#ifdef DEBUG
IObjectSpaceRender* VulkanRenderFactory::CreateObjectSpaceRender()
{
    return new VulkanObjectSpaceRender();
}
void VulkanRenderFactory::DestroyObjectSpaceRender(IObjectSpaceRender* object)
{
    delete object;
}
#endif
#endif
}
