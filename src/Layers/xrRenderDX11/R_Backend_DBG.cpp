#include "stdafx.h"
#pragma hdrstop

namespace xray::render::fg
{

void CBackend::InitializeDebugDraw()
{
}

void CBackend::DestroyDebugDraw()
{
}

void CBackend::dbg_DP(nvrhi::PrimitiveType pt, ref_geom geom, u32 vBase, u32 pc)
{
}

void CBackend::dbg_DIP(nvrhi::PrimitiveType pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
}

#ifdef DEBUG
void CBackend::dbg_Draw(nvrhi::PrimitiveType T, FVF::L* pVerts, u32 vcnt, u16* pIdx, int pcnt)
{
}

void CBackend::dbg_Draw(nvrhi::PrimitiveType T, FVF::L* pVerts, int pcnt)
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

void CBackend::dbg_SetRS(u32, u32)
{
}

void CBackend::dbg_SetSS(u32, u32, u32)
{
}

}
