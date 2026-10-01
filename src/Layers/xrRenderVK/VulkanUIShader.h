#pragma once

#include "xrEngine/Render.h"
#include "Include/xrRender/UIShader.h"
#include "GameTextureFactory.h"

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

private:
    GameTextureFactory* textures_{};
    ScenePass* pass_{};
    std::string shader_, texture_;
    VkDescriptorSet descriptor_{};
    VkExtent2D extent_{};
};
}
