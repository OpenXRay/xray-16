#include "stdafx.h"

void CRenderTarget::phase_rain(CBackend& cmd_list)
{
    // TODO: move into rain render
    u_setrt(cmd_list, rt_Color /*rt_Normal*/, nullptr, nullptr, rt_MSAADepth);
    RImplementation.rmNormal(cmd_list);
}
