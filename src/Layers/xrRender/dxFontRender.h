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
    void CreateFontAtlas(u32 width, u32 height, const char* name, void* bitmap) override;

private:
    ref_shader pShader;
    ref_geom pGeom;
    ref_texture pTexture;

    u32 m_atlasWidth = 0;
    u32 m_atlasHeight = 0;
    u32 m_oglTextureID = 0;
};
} // namespace xray::render::RENDER_NAMESPACE
