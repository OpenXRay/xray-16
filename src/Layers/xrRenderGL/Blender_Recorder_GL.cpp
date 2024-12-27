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
    if (!Enable) return;
    RS.SetRS(D3DRS_STENCILFUNC, Func);
    RS.SetRS(D3DRS_STENCILMASK, Mask);
    RS.SetRS(D3DRS_STENCILWRITEMASK, WriteMask);
    RS.SetRS(D3DRS_STENCILFAIL, Fail);
    RS.SetRS(D3DRS_STENCILPASS, Pass);
    RS.SetRS(D3DRS_STENCILZFAIL, ZFail);
}

void CBlender_Compile::r_StencilRef(u32 Ref)
{
    RS.SetRS(D3DRS_STENCILREF, Ref);
}

void CBlender_Compile::r_CullMode(D3DCULL Mode)
{
    RS.SetRS(D3DRS_CULLMODE, (u32)Mode);
}

void CBlender_Compile::i_Comparison(u32 s, u32 func)
{
    RS.SetSAMP(s, XRDX11SAMP_COMPARISONFILTER, TRUE);
    RS.SetSAMP(s, XRDX11SAMP_COMPARISONFUNC, func);
}

void CBlender_Compile::r_Sampler_cmp(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
    u32 s = r_Sampler(name, texture, b_ps1x_ProjectiveDivide, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE,
                      D3DTEXF_LINEAR);
    if (u32(-1) != s)
    {
        RS.SetSAMP(s, XRDX11SAMP_COMPARISONFILTER, TRUE);
        RS.SetSAMP(s, XRDX11SAMP_COMPARISONFUNC, (u32)D3D_COMPARISON_LESS_EQUAL);
    }
}

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _gs, LPCSTR _ps, bool bFog, BOOL bZtest, BOOL bZwrite, BOOL bABlend,
                              D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
{
    R_ASSERT2(_ps, "Probably you are using wrong r_Pass");
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
    dest.pp = RImplementation.Resources->_CreatePP(_vs, _ps, _gs, "null", "null");
    if (GLAD_GL_ARB_separate_shader_objects || !dest.pp->pp)
#endif
    {
        dest.ps = RImplementation.Resources->_CreatePS(_ps);
        dest.vs = RImplementation.Resources->_CreateVS(_vs);
        dest.gs = RImplementation.Resources->_CreateGS(_gs);
        ctable.merge(&dest.ps->constants);
        ctable.merge(&dest.vs->constants);
        ctable.merge(&dest.gs->constants);
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
