#include "stdafx.h"

namespace xray::render::RENDER_NAMESPACE
{
void CRenderTarget::phase_occq()
{
    if (!RImplementation.o.msaa)
        u_setrtzb(RCache, get_base_rt(), rt_Base_Depth);
    else
        u_setrtzb(RCache, rt_Generic_0_r, rt_MSAADepth);
    RCache.set_Shader(s_occq);
    RCache.set_CullMode(CULL_CCW);
    RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
    RCache.set_ColorWriteEnable(FALSE);
}
} // namespace xray::render::RENDER_NAMESPACE
