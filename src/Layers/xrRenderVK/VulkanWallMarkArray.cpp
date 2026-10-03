#include "xrEngine/stdafx.h"
#include "VulkanWallMarkArray.h"
#include "VulkanGameDevice.h"

namespace xray::render::vulkan
{
void VulkanWallMarkArray::Copy(IWallMarkArray& source)
{
    const auto* other = dynamic_cast<VulkanWallMarkArray*>(&source);
    R_ASSERT2(other && &other->device_ == &device_,
        "Vulkan wallmark arrays must belong to the same device");
    if (other != this) textures_ = other->textures_;
}

void VulkanWallMarkArray::AppendMark(LPCSTR textures)
{
    if (textures && *textures) textures_.emplace_back(textures);
}

const std::string* VulkanWallMarkArray::select_texture() const
{
    return textures_.empty() ? nullptr :
        &textures_[::Random.randI(0, static_cast<int>(textures_.size()))];
}

wm_shader VulkanWallMarkArray::GenerateWallmark()
{
    wm_shader shader;
    if (const std::string* texture = select_texture())
        shader->create("effects\\wallmark", texture->c_str());
    return shader;
}
}
