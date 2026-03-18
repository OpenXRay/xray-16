#include "stdafx.h"

namespace xray::render::RENDER_NAMESPACE
{
void CRenderTarget::phase_occq()
{
#ifdef USE_OGL
    auto rt1 = RImplementation.o.msaa ? rt_Generic_0_r : get_base_rt();
    auto zb = RImplementation.o.msaa ? rt_MSAADepth : rt_Base_Depth;
    u_setrtzb(RCache, rt1, zb);
#else
    if (!RImplementation.o.msaa)
        u_setrt(RCache, Device.dwWidth, Device.dwHeight, get_base_rt(), 0, 0, rt_MSAADepth);
    else
        u_setrt(RCache, Device.dwWidth, Device.dwHeight, 0, 0, 0, rt_MSAADepth);
#endif
    RCache.set_Shader(s_occq);
    RCache.set_CullMode(CULL_CCW);
    RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
    RCache.set_ColorWriteEnable(FALSE);
}
} // namespace xray::render::RENDER_NAMESPACE
