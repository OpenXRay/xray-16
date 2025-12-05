#pragma once

#include "Include/xrRender/UIShader.h"

namespace xray::render::ui {
    class UIRenderCollector;
}

namespace xray::render::framegraph {
    struct ExtractedReflection;
}

namespace xray::render::RENDER_NAMESPACE
{
class dxUIShader : public IUIShader
{
    friend class dxUIRender;
    friend class dxDebugRender;
    friend class dxWallMarkArray;
    friend class CRender;
    friend class xray::render::ui::UIRenderCollector;

public:
    virtual void Copy(IUIShader& _in);
    virtual void create(LPCSTR sh, LPCSTR tex = nullptr);
    virtual bool inited()
    {
        if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12)
            return m_vsHandle && m_psHandle;
        return hShader;
    }
    virtual void destroy();

    bool operator==(const IUIShader& other) const override;

    CTexture* GetBaseTexture() const;
    bool GetBaseTextureResolution(Fvector2& res) override;
    xrImTextureData GetImGuiTextureId() override;

    ref_shader hShader;

    nvrhi::ShaderHandle m_vsHandle;
    nvrhi::ShaderHandle m_psHandle;
    framegraph::ExtractedReflection* m_vsReflection = nullptr;
    framegraph::ExtractedReflection* m_psReflection = nullptr;
    CTexture* m_baseTexture = nullptr;

    shared_str baseTexture{ "s_base" };
};
} // namespace xray::render::RENDER_NAMESPACE
