#pragma once

#include "Include/xrRender/FontRender.h"
#include "VulkanUIShader.h"

namespace xray::render::vulkan
{
class VulkanGameDevice;

class VulkanFontRender final : public IFontRender
{
public:
    explicit VulkanFontRender(VulkanGameDevice& device);
    void Initialize(cpcstr shader, cpcstr texture) override;
    void OnRender(CGameFont& owner) override;

private:
    void glyph(CGameFont& owner, u16 codepoint, float& x, float y,
        float height, u32 top, u32 bottom);
    VulkanGameDevice& device_;
    VulkanUIShader shader_;
};
}
