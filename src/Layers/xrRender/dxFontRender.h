#pragma once

#include "Include/xrRender/FontRender.h"

#include "xrEngine/GameFont.h"

namespace xray::render::framegraph {
    struct ExtractedReflection;  // Forward declaration
}

namespace xray::render::fg
{
class dxFontRender : public IFontRender
{
    friend class xray::render::passes::TextPass;  // Allow TextPass to access shader
public:
    dxFontRender() = default;
    ~dxFontRender() override;

    void Initialize(cpcstr cShader, cpcstr cTexture) override;
    void OnRender(CGameFont& owner) override;

    // DX12: NVRHI shader handles + reflection
    nvrhi::ShaderHandle m_vsHandle;
    nvrhi::ShaderHandle m_psHandle;
    framegraph::ExtractedReflection* m_vsReflection = nullptr;  // Owned by dxFontRender
    framegraph::ExtractedReflection* m_psReflection = nullptr;  // Owned by dxFontRender

    // Stored texture/shader names for FrameGraph access
    shared_str m_shaderName;
    shared_str m_textureName;
    CTexture* m_baseTexture = nullptr;  // DX12: Cached base texture pointer
};
} // namespace xray::render::fg
