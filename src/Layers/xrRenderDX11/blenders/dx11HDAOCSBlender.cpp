#include "stdafx.h"
#include "dx11HDAOCSBlender.h"

namespace xray::render::RENDER_NAMESPACE
{
void CBlender_CS_HDAO::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0:
        C.r_ComputePass("ssao_hdao");

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();

        break;
    }
}

void CBlender_CS_HDAO_MSAA::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0:
        C.r_ComputePass("ssao_hdao_msaa");

        C.r_dx11Texture("s_position", r2_RT_P);
        C.r_dx11Texture("s_normal", r2_RT_N);
        C.r_dx11Sampler("smp_nofilter");
        C.r_End();

        break;
    }
}
} // namespace xray::render::RENDER_NAMESPACE
