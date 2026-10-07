#pragma once

#include "Include/xrRender/ImGuiRender.h"
#include "GameTextureFactory.h"
#include "imgui.h"

#include <unordered_map>

namespace xray::render::vulkan
{
class VulkanGameDevice;

class VulkanImGuiRender final : public IImGuiRender
{
public:
    explicit VulkanImGuiRender(VulkanGameDevice& device) : device_(device) {}
    ~VulkanImGuiRender() override { OnDeviceDestroy(); }
    void Copy(IImGuiRender& source) override;
    void Frame() override;
    void Render(ImDrawData* data) override;
    void OnDeviceCreate(ImGuiContext* context) override;
    void OnDeviceDestroy() override;
    void OnDeviceResetBegin() override;
    void OnDeviceResetEnd() override;

private:
    void update_textures(ImDrawData* data);
    VulkanGameDevice& device_;
    ImGuiContext* context_{};
    std::unordered_map<ImTextureData*, VkDescriptorSet> atlas_;
};
}
