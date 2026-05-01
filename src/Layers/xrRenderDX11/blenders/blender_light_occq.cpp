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
    case 0: // occlusion testing
        C.r_Pass("dumb", "dumb", false, TRUE, FALSE, FALSE);
        C.r_End();
        break;
    case 1: // NV40 optimization :)
        C.r_Pass("null", "dumb", false, FALSE, FALSE, FALSE);
        C.r_End();
        break;
    }
#else
    switch (C.iElement)
    {
    case 0: // occlusion testing
        C.r_Pass("dumb", "dumb", false, TRUE, FALSE, FALSE);
        C.r_End();
        // Color write as well as culling and stencil are set up manually in code.
        break;
    case 1: // NV40 optimization :)
        C.r_Pass("stub_notransform_t", "dumb", false, FALSE, FALSE, FALSE);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_CullMode(nvrhi::RasterCullMode::None);
        C.r_Stencil(TRUE, nvrhi::ComparisonFunc::LessOrEqual, 0xff, 0x00); // keep/keep/keep
        C.r_End();
        break;
    case 2: // Stencil clear in case we've ran out of markers.
        C.r_Pass("stub_notransform_t", "dumb", false, FALSE, FALSE, FALSE);
        C.r_ColorWriteEnable(false, false, false, false);
        C.r_CullMode(nvrhi::RasterCullMode::None);
        if (RImplementation.o.msaa)
            C.r_Stencil(TRUE, nvrhi::ComparisonFunc::Always, 0x00, 0x7E, nvrhi::StencilOp::Zero, nvrhi::StencilOp::Zero, nvrhi::StencilOp::Zero);
        else
        {
            // Clear all bits except the last one
            C.r_Stencil(TRUE, nvrhi::ComparisonFunc::Always, 0x00, 0xFE, nvrhi::StencilOp::Zero, nvrhi::StencilOp::Zero, nvrhi::StencilOp::Zero);
        }
        // C.r_Stencil(TRUE,nvrhi::ComparisonFunc::Always,0x00,0xFF, nvrhi::StencilOp::Zero, nvrhi::StencilOp::Zero, nvrhi::StencilOp::Zero);
        // keep/keep/keep
        C.r_End();
        break;
    }
#endif
}
} // namespace xray::render::fg
