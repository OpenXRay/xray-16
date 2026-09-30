#include "xrEngine/stdafx.h"
#include "VulkanUIShader.h"
#include "ScenePass.h"

namespace xray::render::vulkan
{
void VulkanUIShader::Copy(IUIShader& source)
{
    const auto* other = dynamic_cast<VulkanUIShader*>(&source);
    R_ASSERT2(other && other->textures_ == textures_ && other->pass_ == pass_,
        "Vulkan UI shaders can only be copied within their owning device");
    shader_ = other->shader_;
    texture_ = other->texture_;
    descriptor_ = other->descriptor_;
    extent_ = other->extent_;
}

void VulkanUIShader::create(LPCSTR shader, LPCSTR texture)
{
    destroy();
    shader_ = shader ? shader : "";
    texture_ = texture ? texture : "";
    if (texture_.empty()) return;
    std::string error;
    if (!textures_->ui(texture_, *pass_, descriptor_, error, &extent_))
    {
        Msg("! [renderer-vulkan] UI texture '%s': %s", texture_.c_str(), error.c_str());
        destroy();
    }
}

void VulkanUIShader::destroy()
{
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
    return descriptor_ != VK_NULL_HANDLE && extent_.width && extent_.height;
}

xrImTextureData VulkanUIShader::GetImGuiTextureId()
{
    // ImGui has a separate renderer and descriptor lifetime. Returning the
    // game UI descriptor as an ImTextureID would be invalid until it is bound.
    return {};
}
}
