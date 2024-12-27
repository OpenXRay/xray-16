#include "stdafx.h"

#include "blender_gasmask_dudv.h"

CBlender_gasmask_dudv::CBlender_gasmask_dudv()
{
    description.CLS = B_BLUR;
}

LPCSTR CBlender_gasmask_dudv::getComment()
{
    return "INTERNAL: beef's nightvision";
}

void CBlender_gasmask_dudv::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    C.r_Pass("stub_screen_space", "gasmask_dudv", FALSE, FALSE, FALSE);
    C.r_dx11Texture("s_image", r2_RT_generic0);
    C.r_dx11Texture("s_mask_droplets", "shaders\\gasmasks\\mask_droplets");

    C.r_dx11Texture("s_mask_nm_1", "shaders\\gasmasks\\mask_nm_1");
    C.r_dx11Texture("s_mask_nm_2", "shaders\\gasmasks\\mask_nm_2");
    C.r_dx11Texture("s_mask_nm_3", "shaders\\gasmasks\\mask_nm_3");
    C.r_dx11Texture("s_mask_nm_4", "shaders\\gasmasks\\mask_nm_4");
    C.r_dx11Texture("s_mask_nm_5", "shaders\\gasmasks\\mask_nm_5");
    C.r_dx11Texture("s_mask_nm_6", "shaders\\gasmasks\\mask_nm_6");
    C.r_dx11Texture("s_mask_nm_7", "shaders\\gasmasks\\mask_nm_7");
    C.r_dx11Texture("s_mask_nm_8", "shaders\\gasmasks\\mask_nm_8");
    C.r_dx11Texture("s_mask_nm_9", "shaders\\gasmasks\\mask_nm_9");
    C.r_dx11Texture("s_mask_nm_10", "shaders\\gasmasks\\mask_nm_10");

    C.r_dx11Sampler("smp_base");
    C.r_dx11Sampler("smp_nofilter");
    C.r_dx11Sampler("smp_rtlinear");
    C.r_End();
}
