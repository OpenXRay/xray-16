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
class fgUIShader : public IUIShader
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

    u32 GetBindlessIndex();

    bool SamePipelineAs(const fgUIShader& other) const
    {
        return m_vsHandle.Get() == other.m_vsHandle.Get()
            && m_psHandle.Get() == other.m_psHandle.Get();
    }

    // Legacy D3D11
    ref_shader hShader;

    // DX12: NVRHI shader handles + reflection
    nvrhi::ShaderHandle m_vsHandle;
    nvrhi::ShaderHandle m_psHandle;
    framegraph::ExtractedReflection* m_vsReflection = nullptr;  // Owned by fgUIShader
    framegraph::ExtractedReflection* m_psReflection = nullptr;  // Owned by fgUIShader
    CTexture* m_baseTexture = nullptr;  // DX12: Cached base texture pointer
    u32 m_bindlessTextureIndex = UINT32_MAX;

    shared_str baseTexture{ "s_base" };
};
} // namespace xray::render::fg
