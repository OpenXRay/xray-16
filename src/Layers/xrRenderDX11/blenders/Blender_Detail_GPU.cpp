#include "stdafx.h"
#pragma hdrstop

#include "Blender_Detail_GPU.h"
#include "uber_deffer.h"

namespace xray::render::fg
{
CBlender_Detail_GPU::CBlender_Detail_GPU()
{
    description.CLS = B_DETAIL; // Reuse detail class ID
}

LPCSTR CBlender_Detail_GPU::getComment()
{
    return "INTERNAL: GPU instanced detail rendering with compute culling";
}

#if RENDER == R_R4
void CBlender_Detail_GPU::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    VERIFY(!C.L_textures.empty());
    LPCSTR texture_path = C.L_textures[0].c_str();

    bool bUseATOC = (RImplementation.o.msaa_alphatest == FrameGraphRenderer::MSAA_ATEST_DX10_0_ATOC);

    switch (C.iElement)
    {
    case 0:
        C.r_Pass("detail_gpu", "detail_gpu", FALSE);
        C.r_dx11Texture("s_base", texture_path);
        C.r_dx11Sampler("smp_base");
        C.r_dx11Sampler("smp_linear");

        C.r_dx11Texture("s_interaction_atlas", "$user$interaction_atlas");
        C.r_dx11Texture("interaction_atlas", "$user$interaction_atlas");     // t1 (VS)
        C.r_dx11Texture("wind_texture", "$user$wind");                 // t2 (VS)
        C.r_dx11Texture("slot_indirection", "$user$indirection");      // t3 (VS)
        C.r_dx11Texture("s_grass_vein", "shaders\\grass_vein");        // t4 (PS)

        if (C.iElement != -1)
        {
            C.i_Address(0, D3D_TEXTURE_ADDRESS_WRAP);
            C.i_dx11FilterAnizo(0, TRUE);
        }

        if (ps_r2_ls_flags_ext.test(R2FLAGEXT_WIREFRAME))
            C.R().SetRS(D3DRS_FILLMODE, D3D_FILL_WIREFRAME);

        C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
        C.r_StencilRef(0x01);
        C.r_CullMode(D3D_CULL_NONE);

        C.r_End();
        break;
    }
}
#else
// R1/R2/R3 - GPU instancing only supported in R4+
void CBlender_Detail_GPU::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
}
#endif
} // namespace xray::render::fg
