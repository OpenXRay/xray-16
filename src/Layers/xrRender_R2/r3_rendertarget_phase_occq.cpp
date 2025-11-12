#include "stdafx.h"

namespace xray::render::RENDER_NAMESPACE
{
void CRenderTarget::phase_occq()
{
    auto& rt1 = RImplementation.o.msaa ? rt_Generic_0_r : get_base_rt();
    auto& zb = RImplementation.o.msaa ? rt_MSAADepth : rt_Base_Depth;
#ifdef USE_OGL
    u_setrtzb(RCache, rt1,zb);
#else
    u_setrt(RCache, rt1, nullptr, nullptr, zb);
#endif
    RCache.set_Shader(s_occq);
    RCache.set_CullMode(CULL_CCW);
    RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
    RCache.set_ColorWriteEnable(FALSE);
}
} // namespace xray::render::RENDER_NAMESPACE
