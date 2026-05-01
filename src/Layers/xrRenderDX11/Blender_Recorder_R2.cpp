#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Blender_Recorder.h"
#include "Blender.h"

namespace xray::render::fg
{
void fix_texture_name(pstr fn);

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _ps, bool bFog, BOOL bZtest, BOOL bZwrite,
    BOOL bABlend, nvrhi::BlendFactor abSRC, nvrhi::BlendFactor abDST, BOOL aTest, u32 aRef)
{
    RS.Invalidate();
    ctable.clear();
    passTextures.clear();
    passMatrices.clear();
    passConstants.clear();
    dwStage = 0;

    // Setup FF-units (Z-buffer, blender)
    PassSET_ZB(bZtest, bZwrite);
    PassSET_Blend(bABlend, abSRC, abDST, aTest, aRef);
    PassSET_LightFog(FALSE, bFog);

    // Create shaders
    dest.ps = RImplementation.Resources->_CreatePS(_ps);
    u32 flags = 0;
    if (dest.ps->constants.dx9compatibility)
        flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
    dest.vs = RImplementation.Resources->_CreateVS(_vs, flags);
    dest.gs = RImplementation.Resources->_CreateGS("null");
    dest.hs = RImplementation.Resources->_CreateHS("null");
    dest.ds = RImplementation.Resources->_CreateDS("null");
    dest.cs = RImplementation.Resources->_CreateCS("null");

    // Last Stage - disable
    if (0 == xr_stricmp(_ps, "null"))
    {
    }
}

void CBlender_Compile::r_Constant(LPCSTR name, R_constant_setup* s)
{
    R_ASSERT(s);
    ref_constant C = ctable.get(name);
    if (C)
        C->handler = s;
}

void CBlender_Compile::r_ColorWriteEnable(bool cR, bool cG, bool cB, bool cA)
{
    u8 Mask = 0;
    Mask |= cR ? 1u : 0u;
    Mask |= cG ? 2u : 0u;
    Mask |= cB ? 4u : 0u;
    Mask |= cA ? 8u : 0u;

    RS.SetColorWriteMask(0, static_cast<nvrhi::ColorMask>(Mask));
    RS.SetColorWriteMask(1, static_cast<nvrhi::ColorMask>(Mask));
    RS.SetColorWriteMask(2, static_cast<nvrhi::ColorMask>(Mask));
    RS.SetColorWriteMask(3, static_cast<nvrhi::ColorMask>(Mask));
}

u32 CBlender_Compile::i_Sampler(LPCSTR _name) const
{
    string256 name;
    xr_strcpy(name, _name);
    fix_texture_name(name);

    // Find index
    ref_constant C = ctable.get(name, ctable.dx9compatibility ? RC_sampler : u16(-1));
    if (!C)
        return u32(-1);

    R_ASSERT(C->type == RC_sampler);
    u32 stage = C->samp.index;

    // Create texture
    // while (stage>=passTextures.size())	passTextures.push_back		(NULL);
    return stage;
}

void CBlender_Compile::i_Texture(u32 s, LPCSTR name)
{
    if (name)
        passTextures.emplace_back(s, shared_str(name));
}

void CBlender_Compile::i_Projective(u32, bool) {}

void CBlender_Compile::i_Address(u32 s, nvrhi::SamplerAddressMode address)
{
    RS.SetSamplerAddress(s, address);
}
void CBlender_Compile::i_BorderColor(u32 s, u32 color) { RS.SetSamplerBorderColor(s, color); }
void CBlender_Compile::i_Filter_Min(u32 s, SamplerFilter f) { RS.SetSamplerFilterMin(s, f != SamplerFilter::Point); if (f == SamplerFilter::Anisotropic) RS.SetSamplerAnisotropic(s, 16); }
void CBlender_Compile::i_Filter_Mip(u32 s, SamplerFilter f) { RS.SetSamplerFilterMip(s, f != SamplerFilter::Point); }
void CBlender_Compile::i_Filter_Mag(u32 s, SamplerFilter f) { RS.SetSamplerFilterMag(s, f != SamplerFilter::Point); if (f == SamplerFilter::Anisotropic) RS.SetSamplerAnisotropic(s, 16); }
void CBlender_Compile::i_Filter_Aniso(u32 s, u32 level) { RS.SetSamplerAnisotropic(s, level); }
void CBlender_Compile::i_Filter(u32 s, SamplerFilter _min, SamplerFilter _mip, SamplerFilter _mag)
{
    i_Filter_Min(s, _min);
    i_Filter_Mip(s, _mip);
    i_Filter_Mag(s, _mag);
}

u32 CBlender_Compile::r_Sampler(
    LPCSTR _name, LPCSTR texture, bool b_ps1x_ProjectiveDivide, nvrhi::SamplerAddressMode address,
    SamplerFilter fmin, SamplerFilter fmip, SamplerFilter fmag)
{
    dwStage = i_Sampler(_name);
    if (u32(-1) != dwStage)
    {
#if defined(USE_DX11)
        r_dx11Texture(_name, texture, true);
#elif defined(USE_OGL)
        i_Texture(dwStage, texture);
#else
#   error No graphics API selected or enabled!
#endif

        if ((0 == xr_strcmp(_name, "s_base")) && (fmin == SamplerFilter::Linear))
        {
            fmin = SamplerFilter::Anisotropic;
            fmag = SamplerFilter::Anisotropic;
        }

        if ((0 == xr_strcmp(_name, "s_detail")) && (fmin == SamplerFilter::Linear))
        {
            fmin = SamplerFilter::Anisotropic;
            fmag = SamplerFilter::Anisotropic;
        }

        i_Address(dwStage, address);
        i_Filter(dwStage, fmin, fmip, fmag);
        if (dwStage < 4)
            i_Projective(dwStage, b_ps1x_ProjectiveDivide);
    }
    return dwStage;
}

void CBlender_Compile::r_Sampler_rtf(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
    r_Sampler(name, texture, b_ps1x_ProjectiveDivide, nvrhi::SamplerAddressMode::Clamp, SamplerFilter::Point, SamplerFilter::Point, SamplerFilter::Point);
}
void CBlender_Compile::r_Sampler_clf(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
    r_Sampler(name, texture, b_ps1x_ProjectiveDivide, nvrhi::SamplerAddressMode::Clamp, SamplerFilter::Linear, SamplerFilter::Point, SamplerFilter::Linear);
}
void CBlender_Compile::r_Sampler_clw(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
    u32 s = r_Sampler(
        name, texture, b_ps1x_ProjectiveDivide, nvrhi::SamplerAddressMode::Clamp, SamplerFilter::Linear, SamplerFilter::Point, SamplerFilter::Linear);
    if (u32(-1) != s)
        RS.SetSamplerAddressW(s, nvrhi::SamplerAddressMode::Wrap);
}

void CBlender_Compile::r_End()
{
    SetMapping();
    dest.constants = RImplementation.Resources->_CreateConstantTable(ctable);
    dest.state = RImplementation.Resources->_CreateState(RS.GetContainer());
    dest.T = RImplementation.Resources->_CreateTextureList(passTextures);
    dest.C = nullptr;
    dest.M = nullptr;
    SH->passes.push_back(RImplementation.Resources->_CreatePass(dest));
}
} // namespace xray::render::fg
