#pragma once

#include "Include/xrRender/FontRender.h"

#include "xrEngine/GameFont.h"

namespace xray::render::RENDER_NAMESPACE
{
class dxFontRender : public IFontRender
{
public:
    dxFontRender() = default;
    ~dxFontRender() override;

    void Initialize(cpcstr cShader, cpcstr cTexture) override;
    void OnRender(CGameFont& owner) override;

private:
    inline void ImprintChar(Fvector l, const CGameFont& owner, FVF::TL*& v, float& X, float Y2, u32 clr2, float Y, u32 clr, xr_wide_char* wsStr, int j);

private:
    ref_shader pShader;
    ref_geom pGeom;
};
} // namespace xray::render::RENDER_NAMESPACE
