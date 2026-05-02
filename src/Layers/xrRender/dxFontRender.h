#pragma once

#include "Include/xrRender/FontRender.h"

#include "xrEngine/GameFont.h"

namespace xray::render::framegraph {
    struct ExtractedReflection;
}

namespace xray::render::fg
{
class dxFontRender : public IFontRender
{
    friend class xray::render::passes::TextPass;
public:
    dxFontRender() = default;
    ~dxFontRender() override;

    void Initialize(cpcstr cShader, cpcstr cTexture) override;
    void OnRender(CGameFont& owner) override;

    nvrhi::ShaderHandle m_vsHandle;
    nvrhi::ShaderHandle m_psHandle;
    framegraph::ExtractedReflection* m_vsReflection = nullptr;
    framegraph::ExtractedReflection* m_psReflection = nullptr;

    shared_str m_shaderName;
    shared_str m_textureName;
    CTexture* m_baseTexture = nullptr;
};
}
