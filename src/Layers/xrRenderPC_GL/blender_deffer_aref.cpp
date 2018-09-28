#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/uber_deffer.h"
#include "blender_deffer_aref.h"

CBlender_deffer_aref::CBlender_deffer_aref(bool _lmapped) : lmapped(_lmapped)
{
    description.CLS = B_DEFAULT_AREF;
    oAREF.value = 200;
    oAREF.min = 0;
    oAREF.max = 255;
    oBlend.value = FALSE;
    description.version = 1;
}

CBlender_deffer_aref::~CBlender_deffer_aref() { }

void CBlender_deffer_aref::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP (fs,"Alpha ref", xrPID_INTEGER, oAREF);
    xrPWRITE_PROP (fs,"Alpha-blend", xrPID_BOOL, oBlend);
}

void CBlender_deffer_aref::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    if (1 == version)
    {
        xrPREAD_PROP (fs,xrPID_INTEGER, oAREF);
        xrPREAD_PROP (fs,xrPID_BOOL, oBlend);
    }
}

void CBlender_deffer_aref::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    // oBlend.value	= FALSE	;

    if (oBlend.value)
    {
        switch (C.iElement)
        {
        case SE_R2_NORMAL_HQ:
        case SE_R2_NORMAL_LQ:
            if (lmapped)
            {
                C.r_Pass("lmapE", "lmapE", TRUE,TRUE,FALSE,TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE,
                         oAREF.value);
                C.r_Sampler("s_base", C.L_textures[0]);
                C.r_Sampler("s_lmap", C.L_textures[1]);
                C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
                C.r_Sampler("s_env", r2_T_envs0, false, D3DTADDRESS_CLAMP);
                C.r_End();
            }
            else
            {
                C.r_Pass("vert", "vert", TRUE,TRUE,FALSE,TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE,
                         oAREF.value);
                C.r_Sampler("s_base", C.L_textures[0]);
                C.r_End();
            }
            break;
        default:
            break;
        }
    }
    else
    {
        C.SetParams(1, false); //.

        bool bUseATOC = RImplementation.o.dx10_msaa_alphatest == CRender::MSAA_ATEST_DX10_0_ATOC;

        // codepath is the same, only the shaders differ
        // ***only pixel shaders differ***
        switch (C.iElement)
        {
        case SE_R2_NORMAL_HQ: // deffer

            if (bUseATOC)
            {
                uber_deffer(C, true, "base", "base_atoc", true, nullptr, true);
                C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
                            D3DSTENCILOP_KEEP);
                C.r_ColorWriteEnable(false, false, false, false);
                C.r_StencilRef(0x01);
                //	Alpha to coverage.
                C.RS.SetRS(XRDX10RS_ALPHATOCOVERAGE, TRUE);
                C.r_End();
            }


            uber_deffer(C, true, "base", "base", true, nullptr, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_StencilRef(0x01);
            if (bUseATOC) C.RS.SetRS(D3DRS_ZFUNC, D3DCMP_EQUAL);
            C.r_End();
            break;


        case SE_R2_NORMAL_LQ: // deffer

            if (bUseATOC)
            {
                uber_deffer(C, false, "base", "base_atoc", true, nullptr, true);
                C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
                            D3DSTENCILOP_KEEP);
                C.r_StencilRef(0x01);
                C.r_ColorWriteEnable(false, false, false, false);
                //	Alpha to coverage.
                C.RS.SetRS(XRDX10RS_ALPHATOCOVERAGE, TRUE);
                C.r_End();
            }


            uber_deffer(C, false, "base", "base", true, nullptr, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_StencilRef(0x01);
            if (bUseATOC) C.RS.SetRS(D3DRS_ZFUNC, D3DCMP_EQUAL);
            C.r_End();
            break;


        case SE_R2_SHADOW: // smap
            //			if (RImplementation.o.HW_smap)	C.r_Pass	("shadow_direct_base_aref","shadow_direct_base_aref",FALSE,TRUE,TRUE,FALSE,D3DBLEND_ZERO,D3DBLEND_ONE,TRUE,220);
            //			else							C.r_Pass	("shadow_direct_base_aref","shadow_direct_base_aref",FALSE);
            C.r_Pass("shadow_direct_base_aref", "shadow_direct_base_aref", FALSE, TRUE,TRUE,FALSE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_ColorWriteEnable(false, false, false, false);
            C.r_End();
            break;
        }
    }
}
