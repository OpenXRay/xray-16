#include "stdafx.h"

void CRenderTarget::phase_occq()
{
    if (!RImplementation.o.dx10_msaa)
        u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, 0, 0, HW.pBaseZB);
    else
        u_setrt(Device.dwWidth, Device.dwHeight, 0, 0, 0, rt_MSAADepth->pZRT);
    RCache.set_Shader(s_occq);
    RCache.set_CullMode(CULL_CCW);
    RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
    RCache.set_ColorWriteEnable(FALSE);
}
