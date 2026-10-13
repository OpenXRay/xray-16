#pragma once

#include "Include/xrRender/UIShader.h"
#include "xrCommon/xr_smart_pointers.h"

namespace xray::render::ui {
    class UIRenderCollector;
}

namespace xray::render::framegraph {
    struct ExtractedReflection;
}

namespace xray::render::fg
{
class fgUIShader final : public IUIShader
{
    friend class FrameGraphRenderer;
    friend class xray::render::ui::UIRenderCollector;

public:
    static constexpr u32 ALIVE_SENTINEL = 0xF6A1B3C5u;
    static constexpr u32 DEAD_SENTINEL  = 0xDEADF6A1u;

    fgUIShader() : m_aliveSentinel(ALIVE_SENTINEL) {}
    ~fgUIShader() override;
    fgUIShader(const fgUIShader&) = delete;
    fgUIShader& operator=(const fgUIShader&) = delete;

    bool IsAlive() const { return m_aliveSentinel == ALIVE_SENTINEL; }

    void Copy(IUIShader& _in) override;
    void create(LPCSTR sh, LPCSTR tex = nullptr) override;
    bool inited() override
    {
        return m_vsHandle && m_psHandle;
    }
    void destroy() override;

    CTexture* GetBaseTexture() const;
    bool GetBaseTextureResolution(Fvector2& res) override;
    xrImTextureData GetImGuiTextureId() override;

    u32 GetBindlessIndex();

    bool SamePipelineAs(const fgUIShader& other) const
    {
        return m_vsHandle.Get() == other.m_vsHandle.Get()
            && m_psHandle.Get() == other.m_psHandle.Get();
    }

    bool operator==(const IUIShader& other) const override
    {
        const auto& rhs = static_cast<const fgUIShader&>(other);
        return SamePipelineAs(rhs) && m_baseTexture == rhs.m_baseTexture;
    }

    nvrhi::ShaderHandle m_vsHandle;
    nvrhi::ShaderHandle m_psHandle;
    xr_shared_ptr<const framegraph::ExtractedReflection> m_vsReflection;
    xr_shared_ptr<const framegraph::ExtractedReflection> m_psReflection;
    ref_texture m_baseTexture;

private:
    nvrhi::ITexture* LoadBaseTexture();
    void ReleaseBindlessIndex();

    nvrhi::ITexture* m_bindlessTexture = nullptr;
    u32 m_bindlessTextureIndex = UINT32_MAX;
    u32 m_aliveSentinel{ ALIVE_SENTINEL };
};
}
