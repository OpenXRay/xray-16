#include "stdafx.h"

void CRenderTarget::phase_rain()
{
    u_setrt(rt_Color, NULL, NULL, rt_MSAADepth->pZRT);
    //u_setrt	(rt_Normal,NULL,NULL,get_base_zb());
    RImplementation.rmNormal();
}
