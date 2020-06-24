#include "stdafx.h"
#pragma hdrstop

#include "Blender_tree.h"
#include "uber_deffer.h"

CBlender_Tree::CBlender_Tree()
{
    description.CLS = B_TREE;
    description.version = 1;
    oBlend.value = FALSE;
    oNotAnTree.value = FALSE;
}

void CBlender_Tree::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
    xrPWRITE_PROP(fs, "Object LOD", xrPID_BOOL, oNotAnTree);
}

void CBlender_Tree::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
    if (version >= 1)
    {
        xrPREAD_PROP(fs, xrPID_BOOL, oNotAnTree);
    }
}

LPCSTR CBlender_Tree::getComment()
{
    return "LEVEL: trees/bushes";
}

BOOL CBlender_Tree::canBeDetailed()
{
    return TRUE;
}

#if RENDER == R_R2
void CBlender_Tree::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    //*************** codepath is the same, only shaders differ
    LPCSTR tvs = "tree";
    LPCSTR tvs_s = "shadow_direct_tree";
    if (oNotAnTree.value)
    {
        tvs = "tree_s";
        tvs_s = "shadow_direct_tree_s";
    }
    switch (C.iElement)
    {
    case SE_R2_NORMAL_HQ: // deffer
        uber_deffer(C, true, tvs, "base", oBlend.value);
        break;
    case SE_R2_NORMAL_LQ: // deffer
        uber_deffer(C, false, tvs, "base", oBlend.value);
        break;
    case SE_R2_SHADOW: // smap-spot
        //	TODO: DX10: Use dumb shader for shadowmap since shadows are drawn using hardware PCF
        if (oBlend.value)
            C.r_Pass(tvs_s, "shadow_direct_base_aref", FALSE, TRUE, TRUE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE, TRUE, 200);
        else
            C.r_Pass(tvs_s, "shadow_direct_base", FALSE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_End();
        break;
    }
}
#else
void CBlender_Tree::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    //*************** codepath is the same, only shaders differ
    LPCSTR tvs;
    LPCSTR tvs_s;
    if (oNotAnTree.value)	
    { 
        tvs="tree_s";
        if (oBlend.value)
            tvs_s="shadow_direct_tree_s_aref"; 
        else
            tvs_s="shadow_direct_tree_s"; 
    }
    else
    {
        tvs = "tree";
        if (oBlend.value)
            tvs_s="shadow_direct_tree_aref"; 
        else
            tvs_s="shadow_direct_tree"; 
    }

    bool bUseATOC = (oBlend.value && (RImplementation.o.dx10_msaa_alphatest==CRender::MSAA_ATEST_DX10_0_ATOC));

    switch (C.iElement)
    {
    case SE_R2_NORMAL_HQ:   // deffer
        if (bUseATOC)
        {
            uber_deffer(C, true, tvs, "base_atoc", oBlend.value, 0, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_ColorWriteEnable(false, false, false, false);
            C.r_StencilRef(0x01);
            //	Alpha to coverage.
            C.RS.SetRS(XRDX10RS_ALPHATOCOVERAGE, TRUE);
            C.r_End();
        }

        uber_deffer(C, true, tvs, "base", oBlend.value, 0, true);
        C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        C.r_StencilRef(0x01);
        // Need only for ATOC to emulate stencil test
        if (bUseATOC)
            C.RS.SetRS(D3DRS_ZFUNC, D3DCMP_EQUAL);
        C.r_End();
        break;

    case SE_R2_NORMAL_LQ:   // deffer
        if (bUseATOC)
        {
            uber_deffer(C, false, tvs, "base_atoc", oBlend.value, 0, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_StencilRef(0x01);
            C.r_ColorWriteEnable(false, false, false, false);
            //	Alpha to coverage.
            C.RS.SetRS(XRDX10RS_ALPHATOCOVERAGE, TRUE);
            C.r_End();
        }

        uber_deffer(C, false, tvs, "base", oBlend.value, 0, true);
        C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        C.r_StencilRef(0x01);
        // Need only for ATOC to emulate stencil test
        if (bUseATOC)
            C.RS.SetRS(D3DRS_ZFUNC, D3DCMP_EQUAL);
        C.r_End();
        break;

    case SE_R2_SHADOW:  // smap-spot
        // TODO: DX10: Use dumb shader for shadowmap since shadows are drawn using hardware PCF
        if (oBlend.value)
            C.r_Pass(tvs_s, "shadow_direct_base_aref", FALSE, TRUE, TRUE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE, TRUE, 200);
        else
            C.r_Pass(tvs_s, "shadow_direct_base", FALSE);
        C.SampledImage("smp_linear", "s_base", C.L_textures[0]);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_End();
        break;
    }
}
#endif
