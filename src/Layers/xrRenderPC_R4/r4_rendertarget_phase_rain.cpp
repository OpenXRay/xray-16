#include "stdafx.h"

void CRenderTarget::phase_rain()
{
    if (!RImplementation.o.dx10_msaa)
        u_setrt(rt_Color, NULL, NULL, get_base_zb());
    else
        u_setrt(rt_Color, NULL, NULL, rt_MSAADepth->pZRT);
    // u_setrt	(rt_Normal,NULL,NULL,get_base_zb());
    RImplementation.rmNormal();
}
