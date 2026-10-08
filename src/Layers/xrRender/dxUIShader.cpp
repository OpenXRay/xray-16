#include "stdafx.h"
#include "dxUIShader.h"

namespace xray::render::RENDER_NAMESPACE
{
void dxUIShader::Copy(IUIShader& _in) { *this = *((dxUIShader*)&_in); }
void dxUIShader::create(LPCSTR sh, LPCSTR tex) { hShader.create(sh, tex); }
void dxUIShader::destroy() { hShader.destroy(); }

bool dxUIShader::operator==(const IUIShader& other) const
{
    return hShader == static_cast<const dxUIShader&>(other).hShader;
}

CTexture* dxUIShader::GetBaseTexture() const
{
    if (!hShader)
        return nullptr;

    const ref_selement& element = hShader->E[0];
    if (!element || element->passes.empty() || !element->passes[0])
        return nullptr;

    const SPass& pass = *element->passes[0];
    if (!pass.T)
        return nullptr;

    const STextureList& textures = *pass.T;
    if (textures.empty())
        return nullptr;

    const R_constant* sbase = pass.constants ? pass.constants->get(baseTexture)._get() : nullptr;
    if (!sbase)
        return textures.front().second._get();

    const u32 baseTextureStage = sbase->samp.index;
    for (const auto& [stage, texture] : textures)
    {
        if (stage == baseTextureStage)
            return texture._get();
    }

    Msg("! UI shader has no texture bound to s_base stage %u", baseTextureStage);
    return nullptr;
}

xrImTextureData dxUIShader::GetImGuiTextureId()
{
    const auto texture = GetBaseTexture();
    if (!texture)
        return {};

    return
    {
        texture->GetImTextureID(),
        {
            (float)texture->get_Width(),
            (float)texture->get_Height()
        }
    };
}

bool dxUIShader::GetBaseTextureResolution(Fvector2& res)
{
    const auto texture = GetBaseTexture();
    if (!texture)
    {
        res = {};
        return false;
    }

    if (!texture->flags.bLoaded)
        texture->Load();

    const u32 width = texture->get_Width();
    const u32 height = texture->get_Height();
    res = { float(width), float(height) };
    return width != 0 && height != 0;
}
} // namespace xray::render::RENDER_NAMESPACE
