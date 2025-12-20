#pragma once

#include "Include/xrRender/UIShader.h"

namespace xray::render::RENDER_NAMESPACE
{
class dxUIShader : public IUIShader
{
    friend class dxUIRender;
    friend class dxDebugRender;
    friend class dxWallMarkArray;
    friend class CRender;

public:
    virtual void Copy(IUIShader& _in);
    virtual void create(LPCSTR sh, LPCSTR tex = nullptr);
    virtual bool inited() { return hShader; }
    virtual void destroy();

    bool operator==(const IUIShader& other) const override;

    CTexture* GetBaseTexture() const;
    bool GetBaseTextureResolution(Fvector2& res) override;
    xrImTextureData GetImGuiTextureId() override;

private:
    ref_shader hShader;
    shared_str baseTexture{ "s_base" };
};
} // namespace xray::render::RENDER_NAMESPACE
