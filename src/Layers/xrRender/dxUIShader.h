#pragma once

#include "Include/xrRender/UIShader.h"

namespace xray::render::ui {
    class UIRenderCollector;  // Forward declaration
}

namespace xray::render::framegraph {
    struct ExtractedReflection;  // Forward declaration
}

namespace xray::render::fg
{
class dxUIShader : public IUIShader
{
    friend class FrameGraphRenderer;
    friend class xray::render::ui::UIRenderCollector;  // Allow UI collector to access shader

public:
    virtual void Copy(IUIShader& _in);
    virtual void create(LPCSTR sh, LPCSTR tex = nullptr);
    virtual bool inited()
    {
        return m_vsHandle && m_psHandle;
    }
    virtual void destroy();

    CTexture* GetBaseTexture() const;
    bool GetBaseTextureResolution(Fvector2& res) override;
    xrImTextureData GetImGuiTextureId() override;

    // Legacy D3D11
    ref_shader hShader;

    // DX12: NVRHI shader handles + reflection
    nvrhi::ShaderHandle m_vsHandle;
    nvrhi::ShaderHandle m_psHandle;
    framegraph::ExtractedReflection* m_vsReflection = nullptr;  // Owned by dxUIShader
    framegraph::ExtractedReflection* m_psReflection = nullptr;  // Owned by dxUIShader
    CTexture* m_baseTexture = nullptr;  // DX12: Cached base texture pointer

    shared_str baseTexture{ "s_base" };
};
} // namespace xray::render::fg
