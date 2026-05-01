#include "stdafx.h"
#pragma hdrstop

#include "blender_light_occq.h"

namespace xray::render::fg
{
CBlender_light_occq::CBlender_light_occq() { description.CLS = 0; }
CBlender_light_occq::~CBlender_light_occq() {}
void CBlender_light_occq::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

#if RENDER == R_R2
    switch (C.iElement)
    {
    case 0:
        C.r_Pass("dumb", "dumb", false, TRUE, FALSE, FALSE);
        C.r_End();
        break;
    case 1:
        C.r_Pass("null", "dumb", false, FALSE, FALSE, FALSE);
        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case 0:
        C.r_Pass("dumb", "dumb", false, TRUE, FALSE, FALSE);
        C.r_End();
        break;
    case 1:
        C.r_Pass("stub_notransform_t", "dumb", false, FALSE, FALSE, FALSE);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_CullMode(D3D_CULL_NONE);
        C.r_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, 0xff, 0x00);
        C.r_End();
        break;
    case 2:
        C.r_Pass("stub_notransform_t", "dumb", false, FALSE, FALSE, FALSE);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_CullMode(D3D_CULL_NONE);
        if (RImplementation.o.msaa)
            C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0x00, 0x7E, D3D_STENCIL_OP_ZERO, D3D_STENCIL_OP_ZERO, D3D_STENCIL_OP_ZERO);
        else
        {
            C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0x00, 0xFE, D3D_STENCIL_OP_ZERO, D3D_STENCIL_OP_ZERO, D3D_STENCIL_OP_ZERO);
        }
        C.r_End();
        break;
    }
#endif
}
}
