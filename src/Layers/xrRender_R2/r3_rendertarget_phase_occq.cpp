#include "stdafx.h"

void CRenderTarget::phase_occq()
{
    if (!RImplementation.o.msaa)
        u_setrt(RCache, Device.dwWidth, Device.dwHeight, get_base_rt(), 0, 0, rt_MSAADepth);
    else
        u_setrt(RCache, Device.dwWidth, Device.dwHeight, 0, 0, 0, rt_MSAADepth);
    RCache.set_Shader(s_occq);
    RCache.set_CullMode(CULL_CCW);
    RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
    RCache.set_ColorWriteEnable(FALSE);
}
