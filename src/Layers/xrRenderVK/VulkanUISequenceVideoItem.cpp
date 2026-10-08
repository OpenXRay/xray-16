#include "xrEngine/stdafx.h"
#include "VulkanUISequenceVideoItem.h"
#include "VulkanGameDevice.h"
#include "xrEngine/device.h"

namespace xray::render::vulkan
{
void VulkanUISequenceVideoItem::Copy(IUISequenceVideoItem& source)
{
    const auto* other = dynamic_cast<VulkanUISequenceVideoItem*>(&source);
    R_ASSERT2(other && &other->device_ == &device_, "video item belongs to another Vulkan device");
    video_ = other->video_;
}

void VulkanUISequenceVideoItem::CaptureTexture()
{
    auto* shader = device_.ui().current_shader();
    R_ASSERT2(shader && shader->video(), "UI video capture needs an active movie shader");
    video_ = shader->video();
}

void VulkanUISequenceVideoItem::video_Play(BOOL looped, u32 time)
{
    if (video_) video_->play(looped, time == 0xFFFFFFFF ? Device.dwTimeContinual : time);
}
}
