#include "xrEngine/stdafx.h"
#include "VulkanUIShader.h"
#include "ScenePass.h"
#include "xrEngine/device.h"

#include <cstring>

namespace xray::render::vulkan
{
void VulkanUIShader::Copy(IUIShader& source)
{
    const auto* other = dynamic_cast<VulkanUIShader*>(&source);
    R_ASSERT2(other && other->textures_ == textures_ && other->pass_ == pass_,
        "Vulkan UI shaders can only be copied within their owning device");
    if (other == this) return;
    destroy();
    shader_ = other->shader_;
    texture_ = other->texture_;
    video_ = other->video_;
    if (video_)
    {
        descriptor_ = video_->descriptor();
        extent_ = video_->extent();
        return;
    }
    if (other->descriptor_)
    {
        std::string error;
        if (!textures_->ui(texture_, *pass_, descriptor_, error, &extent_))
        {
            Msg("! [renderer-vulkan] UI shader copy '%s': %s", texture_.c_str(), error.c_str());
            destroy();
        }
    }
}

void VulkanUIShader::create(LPCSTR shader, LPCSTR texture)
{
    destroy();
    shader_ = shader ? shader : "";
    texture_ = texture ? texture : "";
    if (texture_.empty()) return;
    string_path video_path;
    if (FS.exist(video_path, "$game_textures$", texture_.c_str(), ".ogm"))
    {
        video_ = std::make_shared<VulkanVideoTexture>(*textures_, *pass_);
        std::string error;
        if (!video_->load(video_path, Device.dwTimeContinual, error))
        {
            Msg("! [renderer-vulkan] UI movie '%s': %s", texture_.c_str(), error.c_str());
            destroy();
            return;
        }
        descriptor_ = video_->descriptor();
        extent_ = video_->extent();
        return;
    }
    std::string error;
    if (!textures_->ui(texture_, *pass_, descriptor_, error, &extent_))
    {
        Msg("! [renderer-vulkan] UI texture '%s': %s", texture_.c_str(), error.c_str());
        destroy();
    }
}

void VulkanUIShader::destroy()
{
    if (descriptor_ && !video_)
        textures_->release_ui(descriptor_, *pass_);
    video_.reset();
    shader_.clear();
    texture_.clear();
    descriptor_ = VK_NULL_HANDLE;
    extent_ = {};
}

bool VulkanUIShader::operator==(const IUIShader& source) const
{
    const auto* other = dynamic_cast<const VulkanUIShader*>(&source);
    return other && textures_ == other->textures_ && pass_ == other->pass_ &&
        shader_ == other->shader_ && texture_ == other->texture_;
}

bool VulkanUIShader::GetBaseTextureResolution(Fvector2& size)
{
    size.set(static_cast<float>(extent_.width), static_cast<float>(extent_.height));
    return inited() && extent_.width && extent_.height;
}

VkDescriptorSet VulkanUIShader::current_descriptor(u32 time)
{
    if (video_)
    {
        video_->update(time);
        descriptor_ = video_->descriptor();
    }
    return descriptor_;
}

xrImTextureData VulkanUIShader::GetImGuiTextureId()
{
    // Both draw paths use ScenePass's sampled UI descriptor layout. ImGui
    // stores the descriptor handle in its texture ID until shader release.
    xrImTextureData result{};
    const VkDescriptorSet set = current_descriptor(Device.dwTimeContinual);
    static_assert(sizeof(set) <= sizeof(result.texture));
    std::memcpy(&result.texture, &set, sizeof(set));
    result.size.set(float(extent_.width), float(extent_.height));
    return result;
}
}
