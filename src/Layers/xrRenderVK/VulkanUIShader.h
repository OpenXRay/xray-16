#pragma once

#include "xrEngine/Render.h"
#include "Include/xrRender/UIShader.h"
#include "GameTextureFactory.h"
#include "VulkanVideoTexture.h"

#include <vector>

namespace xray::render::vulkan
{
class ScenePass;

// Game UI shader identity and its Vulkan sampled-image descriptor. The pass
// owns descriptor allocations; the texture factory owns the image.
class VulkanUIShader final : public IUIShader
{
public:
    VulkanUIShader(GameTextureFactory& textures, ScenePass& pass)
        : textures_(&textures), pass_(&pass) {}
    ~VulkanUIShader() override { destroy(); }
    void Copy(IUIShader& source) override;
    void create(LPCSTR shader, LPCSTR texture = nullptr) override;
    bool inited() override { return descriptor_ != VK_NULL_HANDLE; }
    void destroy() override;
    bool operator==(const IUIShader& other) const override;
    bool GetBaseTextureResolution(Fvector2& size) override;
    xrImTextureData GetImGuiTextureId() override;
    VkDescriptorSet descriptor() const { return descriptor_; }
    int blend_mode() const { return blend_mode_; }
    int alpha_ref() const { return alpha_ref_; }
    const std::string& texture_name() const { return texture_; }
    VkDescriptorSet current_descriptor(u32 time);
    std::shared_ptr<VulkanVideoTexture> video() const { return video_; }

private:
    void use_transparent_fallback();
    GameTextureFactory* textures_{};
    ScenePass* pass_{};
    std::string shader_, texture_;
    VkDescriptorSet descriptor_{};
    VkExtent2D extent_{};
    int blend_mode_{1};
    int alpha_ref_{};
    std::shared_ptr<VulkanVideoTexture> video_;
    struct SequenceFrame { VkDescriptorSet descriptor; VkExtent2D extent; };
    std::vector<SequenceFrame> sequence_;
    u32 sequence_ms_per_frame_{};
    bool sequence_cycled_{};
};
}
