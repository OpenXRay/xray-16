#include "stdafx.h"
#pragma hdrstop

// TODO: Implement NVRHI-based debug drawing
// This replaces the legacy D3D11/RCache debug code

namespace xray::render::fg
{

void CBackend::InitializeDebugDraw()
{
    // TODO: Initialize NVRHI debug drawing resources
}

void CBackend::DestroyDebugDraw()
{
    // TODO: Cleanup NVRHI debug drawing resources
}

void CBackend::dbg_DP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 vBase, u32 pc)
{
    // TODO: Implement via NVRHI
}

void CBackend::dbg_DIP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
    // TODO: Implement via NVRHI
}

#ifdef DEBUG
void CBackend::dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, u32 vcnt, u16* pIdx, int pcnt)
{
    // TODO: Implement via NVRHI
}

void CBackend::dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int pcnt)
{
    // TODO: Implement via NVRHI
}

void CBackend::dbg_DrawOBB(Fmatrix& T, Fvector& half_dim, u32 C)
{
    // TODO: Implement via NVRHI
}

void CBackend::dbg_DrawTRI(Fmatrix& T, Fvector& p1, Fvector& p2, Fvector& p3, u32 C)
{
    // TODO: Implement via NVRHI
}

void CBackend::dbg_DrawLINE(Fmatrix& T, Fvector& p1, Fvector& p2, u32 C)
{
    // TODO: Implement via NVRHI
}

void CBackend::dbg_DrawEllipse(Fmatrix& T, u32 C)
{
    // TODO: Implement via NVRHI
}
#endif

void CBackend::dbg_OverdrawBegin()
{
    // TODO: Implement via NVRHI
}

void CBackend::dbg_OverdrawEnd()
{
    // TODO: Implement via NVRHI
}

void CBackend::dbg_SetRS(D3DRENDERSTATETYPE p1, u32 p2)
{
    // TODO: Implement via NVRHI
}

void CBackend::dbg_SetSS(u32 sampler, D3DSAMPLERSTATETYPE type, u32 value)
{
    // TODO: Implement via NVRHI
}

} // namespace xray::render::fg
