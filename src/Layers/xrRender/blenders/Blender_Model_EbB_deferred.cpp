#include "stdafx.h"
#pragma hdrstop

#include "Blender_Model_EbB.h"
#include "uber_deffer.h"

CBlender_Model_EbB::CBlender_Model_EbB()
{
    description.CLS = B_MODEL_EbB;
    description.version = 0x1;
    xr_strcpy(oT2_Name, "$null");
    xr_strcpy(oT2_xform, "$null");
    oBlend.value = FALSE;
}

void CBlender_Model_EbB::Save(IWriter& fs)
{
    description.version = 0x1;
    IBlender::Save(fs);
    xrPWRITE_MARKER(fs, "Environment map");
    xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
    xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_xform);
    xrPWRITE_PROP(fs, "Alpha-Blend", xrPID_BOOL, oBlend);
}

void CBlender_Model_EbB::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);
    xrPREAD_MARKER(fs);
    xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
    xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
    if (version >= 0x1)
    {
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
    }
}

LPCSTR CBlender_Model_EbB::getComment()
{
    return "MODEL: env^base";
}

#if RENDER == R_R2
void CBlender_Model_EbB::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (oBlend.value)
    {
        // forward
        LPCSTR vsname = 0;
        LPCSTR psname = 0;
        switch (C.iElement)
        {
        case 0:
        case 1:
            vsname = psname = "model_env_lq";
            C.r_Pass(vsname, psname, TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, 0);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_Sampler("s_env", oT2_Name, false, D3DTADDRESS_CLAMP);
            C.r_End();
            break;
        }
    }
    else
    {
        // deferred
        switch (C.iElement)
        {
        case SE_R2_NORMAL_HQ: // deffer
            uber_deffer(C, true, "model", "base", false);
            break;
        case SE_R2_NORMAL_LQ: // deffer
            uber_deffer(C, false, "model", "base", false);
            break;
        case SE_R2_SHADOW: // smap
            if (RImplementation.o.HW_smap)
                C.r_Pass("shadow_direct_model", "dumb", FALSE, TRUE, TRUE, FALSE);
            else
                C.r_Pass("shadow_direct_model", "shadow_direct_base", FALSE);
            C.r_Sampler("s_base", C.L_textures[0]);
            C.r_End();
            break;
        }
    }
}
#elif RENDER == R_GL
void CBlender_Model_EbB::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (oBlend.value)
    {
        // forward
        LPCSTR	vsname = 0;
        LPCSTR	psname = 0;
        switch (C.iElement)
        {
        case 0:
        case 1:
            vsname = psname = "model_env_lq";
            C.r_Pass(vsname, psname, TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, 0);
            C.r_Sampler			("s_base",	C.L_textures[0]);
            C.r_Sampler			("s_env",	oT2_Name,false,D3DTADDRESS_CLAMP);
            C.r_End();
            break;
        }
    }
    else
    {
        // deferred
        switch (C.iElement)
        {
        case SE_R2_NORMAL_HQ: 	// deffer
            uber_deffer(C, true, "model", "base", false, 0, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_StencilRef(0x01);
            C.r_End();
            break;
        case SE_R2_NORMAL_LQ: 	// deffer
            uber_deffer(C, false, "model", "base", false, 0, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_StencilRef(0x01);
            C.r_End();
            break;
        case SE_R2_SHADOW:		// smap
                                //if (RImplementation.o.HW_smap)	C.r_Pass	("shadow_direct_model","dumb",	FALSE,TRUE,TRUE,FALSE);
                                //else							C.r_Pass	("shadow_direct_model","shadow_direct_base",FALSE);
            C.r_Pass("shadow_direct_model", "dumb", FALSE, TRUE, TRUE, FALSE);
            C.r_Sampler		("s_base",C.L_textures[0]);
            C.r_ColorWriteEnable(false, false, false, false);
            C.r_End();
            break;
        }
    }
}
#else
void CBlender_Model_EbB::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (oBlend.value)
    {
        // forward
        LPCSTR vsname = 0;
        LPCSTR psname = 0;
        switch (C.iElement)
        {
        case 0:
        case 1:
            vsname = psname = "model_env_lq";
            C.r_Pass(vsname, psname, TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, 0);
            // C.r_Sampler			("s_base",	C.L_textures[0]);
            // C.r_Sampler			("s_env",	oT2_Name,false,D3DTADDRESS_CLAMP);
            C.r_dx10Texture("s_base", C.L_textures[0]);
            C.r_dx10Texture("s_env", oT2_Name);

            C.r_dx10Sampler("smp_base");
            C.r_dx10Sampler("smp_rtlinear");
            C.r_End();
            break;
        }
    }
    else
    {
        // deferred
        switch (C.iElement)
        {
        case SE_R2_NORMAL_HQ: // deffer
            uber_deffer(C, true, "model", "base", false, 0, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_StencilRef(0x01);
            C.r_End();
            break;
        case SE_R2_NORMAL_LQ: // deffer
            uber_deffer(C, false, "model", "base", false, 0, true);
            C.r_Stencil(TRUE, D3DCMP_ALWAYS, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            C.r_StencilRef(0x01);
            C.r_End();
            break;
        case SE_R2_SHADOW: // smap
            // if (RImplementation.o.HW_smap)	C.r_Pass	("shadow_direct_model","dumb",	FALSE,TRUE,TRUE,FALSE);
            // else							C.r_Pass	("shadow_direct_model","shadow_direct_base",FALSE);
            C.r_Pass("shadow_direct_model", "dumb", FALSE, TRUE, TRUE, FALSE);
            // C.r_Sampler		("s_base",C.L_textures[0]);
            C.r_dx10Texture("s_base", C.L_textures[0]);
            C.r_dx10Sampler("smp_base");
            C.r_dx10Sampler("smp_linear");
            C.r_ColorWriteEnable(false, false, false, false);
            C.r_End();
            break;
        }
    }
}
#endif
