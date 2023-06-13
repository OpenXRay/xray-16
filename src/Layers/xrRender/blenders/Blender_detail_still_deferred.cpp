#include "stdafx.h"
#pragma hdrstop

#include "Blender_detail_still.h"
#include "uber_deffer.h"

CBlender_Detail_Still::CBlender_Detail_Still()
{
    description.CLS = B_DETAIL;
    description.version = 0;
}

void CBlender_Detail_Still::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
}

void CBlender_Detail_Still::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
}

LPCSTR CBlender_Detail_Still::getComment()
{
    return "LEVEL: detail objects";
}

#if RENDER == R_R2
void CBlender_Detail_Still::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case SE_R2_NORMAL_HQ: // deffer wave
        uber_deffer(C, false, "detail_w", "base", true);
        break;
    case SE_R2_NORMAL_LQ: // deffer still
        uber_deffer(C, false, "detail_s", "base", true);
        break;
    }
}
#else
void CBlender_Detail_Still::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    bool bUseATOC = (RImplementation.o.msaa_alphatest == CRender::MSAA_ATEST_DX10_0_ATOC);

    switch (C.iElement)
    {
    case SE_R2_NORMAL_HQ: // deffer wave
        if (bUseATOC)
        {
            uber_deffer(C, false, "detail_w", "base_atoc", true, 0, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_StencilRef(0x01);
            C.r_ColorWriteEnable(false, false, false, false);
            C.r_CullMode(D3DCULL_NONE);
#if defined(USE_DX11)
            if (RImplementation.o.instanced_details)
            {
                C.r_dx11Texture("array", "$details$array");
            }
#endif
            //	Alpha to coverage.
            C.RS.SetRS(XRDX11RS_ALPHATOCOVERAGE, TRUE);
            C.r_End();
        }

        uber_deffer(C, false, "detail_w", "base", true, 0, true);
        C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        C.r_StencilRef(0x01);
        C.r_CullMode(D3DCULL_NONE);
#if defined(USE_DX11)
        if (RImplementation.o.instanced_details)
        {
            C.r_dx11Texture("array", "$details$array");
        }
#endif
        if (bUseATOC)
            C.RS.SetRS(D3DRS_ZFUNC, D3DCMP_EQUAL);
        C.r_End();
        break;
    case SE_R2_NORMAL_LQ: // deffer still
        if (bUseATOC)
        {
            uber_deffer(C, false, "detail_s", "base_atoc", true, 0, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_StencilRef(0x01);
            C.r_CullMode(D3DCULL_NONE);
            C.r_ColorWriteEnable(false, false, false, false);
#if defined(USE_DX11)
            if (RImplementation.o.instanced_details)
            {
                C.r_dx11Texture("array", "$details$array");
            }
#endif
            //	Alpha to coverage.
            C.RS.SetRS(XRDX11RS_ALPHATOCOVERAGE, TRUE);
            C.r_End();
        }

        uber_deffer(C, false, "detail_s", "base", true, 0, true);
        C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        C.r_StencilRef(0x01);
        C.r_CullMode(D3DCULL_NONE);
#if defined(USE_DX11)
        if (RImplementation.o.instanced_details)
        {
            C.r_dx11Texture("array", "$details$array");
        }
#endif
        //	Need this for ATOC
        if (bUseATOC)
            C.RS.SetRS(D3DRS_ZFUNC, D3DCMP_EQUAL);
        C.r_End();
        break;
    }
}
#endif
