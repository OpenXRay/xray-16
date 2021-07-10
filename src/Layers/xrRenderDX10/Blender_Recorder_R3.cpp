#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/Blender_Recorder.h"
#include "Layers/xrRender/Blender.h"
#include "Layers/xrRender/tss.h"

void fix_texture_name(pstr fn);

void CBlender_Compile::r_Stencil(BOOL Enable, u32 Func, u32 Mask, u32 WriteMask, u32 Fail, u32 Pass, u32 ZFail)
{
    RS.SetRS(D3DRS_STENCILENABLE, BC(Enable));
    if (!Enable)
        return;
    RS.SetRS(D3DRS_STENCILFUNC, Func);
    RS.SetRS(D3DRS_STENCILMASK, Mask);
    RS.SetRS(D3DRS_STENCILWRITEMASK, WriteMask);
    RS.SetRS(D3DRS_STENCILFAIL, Fail);
    RS.SetRS(D3DRS_STENCILPASS, Pass);
    RS.SetRS(D3DRS_STENCILZFAIL, ZFail);
    //	Since we never really support different options for
    //	CW/CCW stencil use it to mimic DX9 behaviour for
    //	single-sided stencil
    RS.SetRS(D3DRS_CCW_STENCILFUNC, Func);
    RS.SetRS(D3DRS_CCW_STENCILFAIL, Fail);
    RS.SetRS(D3DRS_CCW_STENCILPASS, Pass);
    RS.SetRS(D3DRS_CCW_STENCILZFAIL, ZFail);
}

void CBlender_Compile::r_StencilRef(u32 Ref) { RS.SetRS(D3DRS_STENCILREF, Ref); }
void CBlender_Compile::r_CullMode(D3DCULL Mode) { RS.SetRS(D3DRS_CULLMODE, (u32)Mode); }
void CBlender_Compile::r_dx10Texture(LPCSTR ResourceName, LPCSTR texture, bool recursive /*= false*/)
{
    if (ctable.dx9compatibility && !recursive)
    {
        const u32 stage = r_Sampler(ResourceName, texture);
        if (stage != u16(-1))
            return;
    }

    VERIFY(ResourceName);
    if (!texture)
        return;
    //
    string256 TexName;
    xr_strcpy(TexName, texture);
    fix_texture_name(TexName);

    // Find index
    ref_constant C = ctable.get(ResourceName, ctable.dx9compatibility ? RC_dx10texture : u16(-1));
    // VERIFY(C);
    if (!C)
        return;

    R_ASSERT(C->type == RC_dx10texture);
    u32 stage = C->samp.index;

    passTextures.push_back(std::make_pair(stage, ref_texture(RImplementation.Resources->_CreateTexture(TexName))));
}

void CBlender_Compile::i_dx10FilterAnizo(u32 s, BOOL value)
{
    VERIFY(s != u32(-1));
    RS.SetSAMP(s, XRDX10SAMP_ANISOTROPICFILTER, value);
}

u32 CBlender_Compile::r_dx10Sampler(LPCSTR ResourceName)
{
    // TODO: DX10: Check if we can use dwStage
    u32 stage = i_Sampler(ResourceName);

    if (stage == u32(-1))
        return u32(-1);

    //	init defaults here:

    //	Use D3DTADDRESS_CLAMP,	D3DTEXF_POINT,			D3DTEXF_NONE,	D3DTEXF_POINT
    if (0 == xr_strcmp(ResourceName, "smp_nofilter"))
    {
        i_Address(stage, D3DTADDRESS_CLAMP);
        i_Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    }

    //	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR
    else if (0 == xr_strcmp(ResourceName, "smp_rtlinear"))
    {
        i_Address(stage, D3DTADDRESS_CLAMP);
        i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
    }

    //	Use	D3DTADDRESS_WRAP,	D3DTEXF_LINEAR,			D3DTEXF_LINEAR,	D3DTEXF_LINEAR
    else if (0 == xr_strcmp(ResourceName, "smp_linear"))
    {
        i_Address(stage, D3DTADDRESS_WRAP);
        i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }

    //	Use D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC, 	D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC
    else if (0 == xr_strcmp(ResourceName, "smp_base"))
    {
        i_Address(stage, D3DTADDRESS_WRAP);
        i_dx10FilterAnizo(stage, TRUE);
        // i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }

    //	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR
    else if (0 == xr_strcmp(ResourceName, "smp_material"))
    {
        i_Address(stage, D3DTADDRESS_CLAMP);
        i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
        RS.SetSAMP(stage, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
    }

    else if (0 == xr_strcmp(ResourceName, "smp_smap"))
    {
        i_Address(stage, D3DTADDRESS_CLAMP);
        i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
        RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFILTER, TRUE);
        RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFUNC, (u32)D3D_COMPARISON_LESS_EQUAL);
    }

    else if (0 == xr_strcmp(ResourceName, "smp_jitter"))
    {
        i_Address(stage, D3DTADDRESS_WRAP);
        i_Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    }

    return stage;
}

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _gs, LPCSTR _ps, bool bFog, BOOL bZtest, BOOL bZwrite, BOOL bABlend,
                              D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
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
    SPS* ps = RImplementation.Resources->_CreatePS(_ps);
    u32 flags = 0;
    if (ps->constants.dx9compatibility)
        flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
    SVS* vs = RImplementation.Resources->_CreateVS(_vs, flags);
    SGS* gs = RImplementation.Resources->_CreateGS(_gs);
    dest.ps = ps;
    dest.vs = vs;
    dest.gs = gs;
#ifdef USE_DX11
    dest.hs = RImplementation.Resources->_CreateHS("null");
    dest.ds = RImplementation.Resources->_CreateDS("null");
    dest.cs = RImplementation.Resources->_CreateCS("null");
#endif
    ctable.merge(&ps->constants);
    ctable.merge(&vs->constants);
    ctable.merge(&gs->constants);

    // Last Stage - disable
    if (0 == xr_stricmp(_ps, "null"))
    {
        RS.SetTSS(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
        RS.SetTSS(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }
}

#ifdef USE_DX11
void CBlender_Compile::r_TessPass(LPCSTR vs, LPCSTR hs, LPCSTR ds, LPCSTR gs, LPCSTR ps, bool bFog, BOOL bZtest,
    BOOL bZwrite, BOOL bABlend, D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
{
    r_Pass(vs, gs, ps, bFog, bZtest, bZwrite, bABlend, abSRC, abDST, aTest, aRef);

    dest.hs = RImplementation.Resources->_CreateHS(hs);
    dest.ds = RImplementation.Resources->_CreateDS(ds);

    ctable.merge(&dest.hs->constants);
    ctable.merge(&dest.ds->constants);
}

void CBlender_Compile::r_ComputePass(LPCSTR cs)
{
    dest.cs = RImplementation.Resources->_CreateCS(cs);

    ctable.merge(&dest.cs->constants);
}
#endif
