#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "Blender_Recorder.h"
#include "Blender.h"

void fix_texture_name(pstr fn);

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _ps, bool bFog, BOOL bZtest, BOOL bZwrite,
    BOOL bABlend, D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
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
#if defined(USE_OGL)
    dest.pp = RImplementation.Resources->_CreatePP(_vs, _ps, "null", "null", "null");
    if (HW.SeparateShaderObjectsSupported || !dest.pp->pp)
#endif
    {
        dest.ps = RImplementation.Resources->_CreatePS(_ps);
        ctable.merge(&dest.ps->constants);
        u32 flags = 0;
#if defined(USE_DX11) || defined(USE_DX12)
        if (dest.ps->constants.dx9compatibility)
            flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
#endif
        dest.vs = RImplementation.Resources->_CreateVS(_vs, flags);
        ctable.merge(&dest.vs->constants);
#if defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
        dest.gs = RImplementation.Resources->_CreateGS("null");
#if defined(USE_DX11) || defined(USE_DX12)
        dest.hs = RImplementation.Resources->_CreateHS("null");
        dest.ds = RImplementation.Resources->_CreateDS("null");
        dest.cs = RImplementation.Resources->_CreateCS("null");
#    endif
#endif // !USE_DX9
    }
#if defined(USE_OGL)
    RImplementation.Resources->_LinkPP(dest);
    ctable.merge(&dest.pp->constants);
#endif

    // Last Stage - disable
    if (0 == xr_stricmp(_ps, "null"))
    {
        RS.SetTSS(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
        RS.SetTSS(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
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
    Mask |= cR ? D3DCOLORWRITEENABLE_RED : 0;
    Mask |= cG ? D3DCOLORWRITEENABLE_GREEN : 0;
    Mask |= cB ? D3DCOLORWRITEENABLE_BLUE : 0;
    Mask |= cA ? D3DCOLORWRITEENABLE_ALPHA : 0;

    RS.SetRS(D3DRS_COLORWRITEENABLE, Mask);
    RS.SetRS(D3DRS_COLORWRITEENABLE1, Mask);
    RS.SetRS(D3DRS_COLORWRITEENABLE2, Mask);
    RS.SetRS(D3DRS_COLORWRITEENABLE3, Mask);
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
        passTextures.emplace_back(s, ref_texture(RImplementation.Resources->_CreateTexture(name)));
}

void CBlender_Compile::i_Projective(u32 s, bool b)
{
    if (b)
        RS.SetTSS(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE | D3DTTFF_PROJECTED);
    else
        RS.SetTSS(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void CBlender_Compile::i_Address(u32 s, u32 address)
{
    RS.SetSAMP(s, D3DSAMP_ADDRESSU, address);
    RS.SetSAMP(s, D3DSAMP_ADDRESSV, address);
    RS.SetSAMP(s, D3DSAMP_ADDRESSW, address);
}
void CBlender_Compile::i_BorderColor(u32 s, u32 color) { RS.SetSAMP(s, D3DSAMP_BORDERCOLOR, color); }
void CBlender_Compile::i_Filter_Min(u32 s, u32 f) { RS.SetSAMP(s, D3DSAMP_MINFILTER, f); }
void CBlender_Compile::i_Filter_Mip(u32 s, u32 f) { RS.SetSAMP(s, D3DSAMP_MIPFILTER, f); }
void CBlender_Compile::i_Filter_Mag(u32 s, u32 f) { RS.SetSAMP(s, D3DSAMP_MAGFILTER, f); }
void CBlender_Compile::i_Filter_Aniso(u32 s, u32 f) { RS.SetSAMP(s, D3DSAMP_MAXANISOTROPY, f); }
void CBlender_Compile::i_Filter(u32 s, u32 _min, u32 _mip, u32 _mag)
{
    i_Filter_Min(s, _min);
    i_Filter_Mip(s, _mip);
    i_Filter_Mag(s, _mag);
#if defined(USE_OGL)
    if (_min == D3DTEXF_ANISOTROPIC && _mag == D3DTEXF_ANISOTROPIC)
        i_Filter_Aniso(s, ps_r__tf_Anisotropic);
#endif
}

u32 CBlender_Compile::r_Sampler(
    LPCSTR _name, LPCSTR texture, bool b_ps1x_ProjectiveDivide, u32 address, u32 fmin, u32 fmip, u32 fmag)
{
    dwStage = i_Sampler(_name);
    if (u32(-1) != dwStage)
    {
#if defined(USE_DX11) || defined(USE_DX12)
        r_dx11Texture(_name, texture, true);
#elif defined(USE_DX9) || defined(USE_OGL)
        i_Texture(dwStage, texture);
#else
#   error No graphics API selected or enabled!
#endif

        // force ANISO-TF for "s_base"
        if ((0 == xr_strcmp(_name, "s_base")) && (fmin == D3DTEXF_LINEAR))
        {
            fmin = D3DTEXF_ANISOTROPIC;
            fmag = D3DTEXF_ANISOTROPIC;
        }

        if (0 == xr_strcmp(_name, "s_base_hud"))
        {
            fmin = D3DTEXF_GAUSSIANQUAD; // D3DTEXF_PYRAMIDALQUAD; //D3DTEXF_ANISOTROPIC; //D3DTEXF_LINEAR;
            // //D3DTEXF_POINT; //D3DTEXF_NONE
            fmag = D3DTEXF_GAUSSIANQUAD; // D3DTEXF_PYRAMIDALQUAD; //D3DTEXF_ANISOTROPIC; //D3DTEXF_LINEAR;
            // //D3DTEXF_POINT; //D3DTEXF_NONE;
        }

        if ((0 == xr_strcmp(_name, "s_detail")) && (fmin == D3DTEXF_LINEAR))
        {
            fmin = D3DTEXF_ANISOTROPIC;
            fmag = D3DTEXF_ANISOTROPIC;
        }

#if defined(USE_OGL)
        if (0 == xr_strcmp(_name, "s_position"))
        {
            address = D3DTADDRESS_CLAMP;
            fmin = D3DTEXF_POINT;
            fmip = D3DTEXF_NONE;
            fmag = D3DTEXF_POINT;
        }

        if (0 == xr_strcmp(_name, "s_smap"))
        {
            address = D3DTADDRESS_CLAMP;
            fmip = D3DTEXF_NONE;
        }
#endif

        // Sampler states
        i_Address(dwStage, address);
        i_Filter(dwStage, fmin, fmip, fmag);
        //.i_Filter				(dwStage,D3DTEXF_POINT,D3DTEXF_POINT,D3DTEXF_POINT); // show pixels
        if (dwStage < 4)
            i_Projective(dwStage, b_ps1x_ProjectiveDivide);
    }
    return dwStage;
}

void CBlender_Compile::r_Sampler_rtf(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
    r_Sampler(name, texture, b_ps1x_ProjectiveDivide, D3DTADDRESS_CLAMP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
}
void CBlender_Compile::r_Sampler_clf(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
    r_Sampler(name, texture, b_ps1x_ProjectiveDivide, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
}
void CBlender_Compile::r_Sampler_clw(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
    u32 s = r_Sampler(
        name, texture, b_ps1x_ProjectiveDivide, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
    if (u32(-1) != s)
        RS.SetSAMP(s, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
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
