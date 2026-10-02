#include "xrEngine/stdafx.h"
#include "VulkanImGuiRender.h"
#include "VulkanGameDevice.h"

#include <cstring>
#include <vector>

namespace xray::render::vulkan
{
void VulkanImGuiRender::Copy(IImGuiRender& source)
{
    const auto* other = dynamic_cast<VulkanImGuiRender*>(&source);
    R_ASSERT2(other && &other->device_ == &device_, "ImGui renderer belongs to another Vulkan device");
    if (other == this) return;
    OnDeviceDestroy();
    OnDeviceCreate(other->context_);
}

void VulkanImGuiRender::OnDeviceCreate(ImGuiContext* context)
{
    R_ASSERT2(context, "ImGui context is missing");
    context_ = context;
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "xrRenderVK";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;
}

void VulkanImGuiRender::OnDeviceResetBegin()
{
    if (!context_) return;
    ImGui::SetCurrentContext(context_);
    for (auto& [texture, descriptor] : atlas_)
    {
        device_.textures().release_ui(descriptor, device_.ui_pass());
        texture->SetTexID(ImTextureID_Invalid);
        texture->SetStatus(ImTextureStatus_WantCreate);
    }
    atlas_.clear();
}
void VulkanImGuiRender::OnDeviceResetEnd() {}

void VulkanImGuiRender::OnDeviceDestroy()
{
    if (!context_) return;
    OnDeviceResetBegin();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    io.BackendRendererName = nullptr;
    context_ = nullptr;
}

void VulkanImGuiRender::Frame()
{
    if (context_) ImGui::SetCurrentContext(context_);
}

void VulkanImGuiRender::update_textures(ImDrawData* data)
{
    if (!data || !data->Textures) return;
    for (ImTextureData* texture : *data->Textures)
    {
        if (texture->Status == ImTextureStatus_WantDestroy)
        {
            auto it = atlas_.find(texture);
            if (it != atlas_.end())
            {
                device_.textures().release_ui(it->second, device_.ui_pass());
                atlas_.erase(it);
            }
            texture->SetTexID(ImTextureID_Invalid);
            texture->SetStatus(ImTextureStatus_Destroyed);
            continue;
        }
        if (texture->Status != ImTextureStatus_WantCreate &&
            texture->Status != ImTextureStatus_WantUpdates) continue;
        if (!texture->Pixels || texture->Width <= 0 || texture->Height <= 0) continue;
        const auto width = static_cast<uint32_t>(texture->Width);
        const auto height = static_cast<uint32_t>(texture->Height);
        std::vector<uint8_t> rgba;
        const uint8_t* pixels = texture->Pixels;
        if (texture->Format == ImTextureFormat_Alpha8)
        {
            rgba.resize(size_t(width) * height * 4);
            for (size_t pixel = 0; pixel < size_t(width) * height; ++pixel)
            {
                rgba[pixel * 4] = rgba[pixel * 4 + 1] = rgba[pixel * 4 + 2] = 255;
                rgba[pixel * 4 + 3] = pixels[pixel];
            }
            pixels = rgba.data();
        }
        else if (texture->Format != ImTextureFormat_RGBA32) continue;
        VkDescriptorSet descriptor{};
        std::string error;
        if (!device_.textures().ui_pixels(pixels, width, height,
                device_.ui_pass(), descriptor, error))
        {
            Msg("! [renderer-vulkan] ImGui atlas: %s", error.c_str());
            continue;
        }
        auto it = atlas_.find(texture);
        if (it != atlas_.end()) device_.textures().release_ui(it->second, device_.ui_pass());
        atlas_[texture] = descriptor;
        ImTextureID id{};
        static_assert(sizeof(descriptor) <= sizeof(id));
        std::memcpy(&id, &descriptor, sizeof(descriptor));
        texture->SetTexID(id);
        texture->SetStatus(ImTextureStatus_OK);
    }
}

void VulkanImGuiRender::Render(ImDrawData* data)
{
    if (!context_ || !data) return;
    ImGui::SetCurrentContext(context_);
    update_textures(data);
    device_.ui().append_imgui(data);
}
}
