#include "stdafx.h"
#pragma hdrstop

namespace xray::render::RENDER_NAMESPACE
{

void CBackend::InitializeDebugDraw()
{
}

void CBackend::DestroyDebugDraw()
{
}

void CBackend::dbg_DP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 vBase, u32 pc)
{
}

void CBackend::dbg_DIP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
}

#ifdef DEBUG
void CBackend::dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, u32 vcnt, u16* pIdx, int pcnt)
{
}

void CBackend::dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int pcnt)
{
}

void CBackend::dbg_DrawOBB(Fmatrix& T, Fvector& half_dim, u32 C)
{
}

void CBackend::dbg_DrawTRI(Fmatrix& T, Fvector& p1, Fvector& p2, Fvector& p3, u32 C)
{
}

void CBackend::dbg_DrawLINE(Fmatrix& T, Fvector& p1, Fvector& p2, u32 C)
{
}

void CBackend::dbg_DrawEllipse(Fmatrix& T, u32 C)
{
}
#endif

void CBackend::dbg_OverdrawBegin()
{
}

void CBackend::dbg_OverdrawEnd()
{
}

void CBackend::dbg_SetRS(D3DRENDERSTATETYPE p1, u32 p2)
{
}

void CBackend::dbg_SetSS(u32 sampler, D3DSAMPLERSTATETYPE type, u32 value)
{
}

}
