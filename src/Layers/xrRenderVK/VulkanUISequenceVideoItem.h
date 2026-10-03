#pragma once

#include "Include/xrRender/UISequenceVideoItem.h"
#include "VulkanVideoTexture.h"

namespace xray::render::vulkan
{
class VulkanGameDevice;

class VulkanUISequenceVideoItem final : public IUISequenceVideoItem
{
public:
    explicit VulkanUISequenceVideoItem(VulkanGameDevice& device) : device_(device) {}
    void Copy(IUISequenceVideoItem& source) override;
    bool HasTexture() override { return !!video_; }
    void CaptureTexture() override;
    void ResetTexture() override { video_.reset(); }
    BOOL video_IsPlaying() override { return video_ && video_->playing(); }
    void video_Sync(u32 time) override { if (video_) video_->sync(time); }
    void video_Play(BOOL looped, u32 time = 0xFFFFFFFF) override;
    void video_Stop() override { if (video_) video_->stop(); }

private:
    VulkanGameDevice& device_;
    std::shared_ptr<VulkanVideoTexture> video_;
};
}
