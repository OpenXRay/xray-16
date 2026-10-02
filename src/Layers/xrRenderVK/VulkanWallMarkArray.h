#pragma once

#include "Include/xrRender/WallMarkArray.h"

#include <string>
#include <vector>

namespace xray::render::vulkan
{
class VulkanGameDevice;

// Material selection is kept independent of the marks placed in the scene.
// A generated shader owns its own descriptor, so clearing this array does not
// invalidate a mark already submitted to the renderer.
class VulkanWallMarkArray final : public IWallMarkArray
{
public:
    explicit VulkanWallMarkArray(VulkanGameDevice& device) : device_(device) {}
    void Copy(IWallMarkArray& source) override;
    void AppendMark(LPCSTR textures) override;
    void clear() override { textures_.clear(); }
    bool empty() override { return textures_.empty(); }
    wm_shader GenerateWallmark() override;
    const std::string* select_texture() const;

private:
    VulkanGameDevice& device_;
    std::vector<std::string> textures_;
};
}
