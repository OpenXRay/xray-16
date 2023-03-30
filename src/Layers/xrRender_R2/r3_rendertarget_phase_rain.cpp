#include "stdafx.h"

void CRenderTarget::phase_rain()
{
    u_setrt(rt_Color /*rt_Normal*/, nullptr, nullptr, rt_MSAADepth);
    RImplementation.rmNormal();
}
