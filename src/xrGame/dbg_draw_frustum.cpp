#include "StdAfx.h"
#include "Level.h"
#include "xrCDB/Frustum.h"

#ifdef DEBUG
#include "debug_renderer.h"
#endif

#ifdef DEBUG
void MK_Frustum(CFrustum& F, float FOV, float _FAR, float A, Fvector& P, Fvector& D, Fvector& U)
{
    float YFov = deg2rad(FOV);
    float XFov = deg2rad(FOV / A);

    // calc window extents in camera coords
    float wR = tanf(XFov * 0.5f);
    float wL = -wR;
    float wT = tanf(YFov * 0.5f);
    float wB = -wT;

    // calc x-axis (viewhoriz) and store cop
    // here we are assuring that vectors are perpendicular & normalized
    Fvector R, COP;
    D.normalize();
    R.crossproduct(D, U);
    R.normalize();
    U.crossproduct(R, D);
    U.normalize();
    COP.set(P);

    // calculate the corner vertices of the window
    Fvector sPts[4]; // silhouette points (corners of window)
    Fvector Offset, T;
    Offset.add(D, COP);

    sPts[0].mul(R, wR);
    T.mad(Offset, U, wT);
    sPts[0].add(T);
    sPts[1].mul(R, wL);
    T.mad(Offset, U, wT);
    sPts[1].add(T);
    sPts[2].mul(R, wL);
    T.mad(Offset, U, wB);
    sPts[2].add(T);
    sPts[3].mul(R, wR);
    T.mad(Offset, U, wB);
    sPts[3].add(T);

    // find projector direction vectors (from cop through silhouette pts)
    Fvector ProjDirs[4];
    ProjDirs[0].sub(sPts[0], COP);
    ProjDirs[1].sub(sPts[1], COP);
    ProjDirs[2].sub(sPts[2], COP);
    ProjDirs[3].sub(sPts[3], COP);

    Fvector _F[4];
    _F[0].mad(COP, ProjDirs[0], _FAR);
    _F[1].mad(COP, ProjDirs[1], _FAR);
    _F[2].mad(COP, ProjDirs[2], _FAR);
    _F[3].mad(COP, ProjDirs[3], _FAR);

    F.CreateFromPoints(_F, 4, COP);
}

void dbg_draw_frustum(float FOV, float _FAR, float A, Fvector& P, Fvector& D, Fvector& U)
{
    // if (!bDebug)		return;

    float YFov = deg2rad(FOV * A);
    float XFov = deg2rad(FOV);

    // calc window extents in camera coords
    float wR = tanf(XFov * 0.5f);
    float wL = -wR;
    float wT = tanf(YFov * 0.5f);
    float wB = -wT;

    // calc x-axis (viewhoriz) and store cop
    // here we are assuring that vectors are perpendicular & normalized
    Fvector R, COP;
    D.normalize();
    R.crossproduct(D, U);
    R.normalize();
    U.crossproduct(R, D);
    U.normalize();
    COP.set(P);

    // calculate the corner vertices of the window
    Fvector sPts[4]; // silhouette points (corners of window)
    Fvector Offset, T;
    Offset.add(D, COP);

    sPts[0].mul(R, wR);
    T.mad(Offset, U, wT);
    sPts[0].add(T);
    sPts[1].mul(R, wL);
    T.mad(Offset, U, wT);
    sPts[1].add(T);
    sPts[2].mul(R, wL);
    T.mad(Offset, U, wB);
    sPts[2].add(T);
    sPts[3].mul(R, wR);
    T.mad(Offset, U, wB);
    sPts[3].add(T);

    // find projector direction vectors (from cop through silhouette pts)
    Fvector ProjDirs[4];
    ProjDirs[0].sub(sPts[0], COP);
    ProjDirs[1].sub(sPts[1], COP);
    ProjDirs[2].sub(sPts[2], COP);
    ProjDirs[3].sub(sPts[3], COP);

    // RCache.set_CullMode	(CULL_NONE);
    GEnv.DRender->CacheSetCullMode(IDebugRender::cmNONE);
    // CHK_DX(HW.pDevice->SetRenderState	(D3DRS_AMBIENT,		0xffffffff			));
    GEnv.DRender->SetAmbient(0xffffffff);

    Fvector _F[4];
    _F[0].mad(COP, ProjDirs[0], _FAR);
    _F[1].mad(COP, ProjDirs[1], _FAR);
    _F[2].mad(COP, ProjDirs[2], _FAR);
    _F[3].mad(COP, ProjDirs[3], _FAR);

    //	u32 CT	= color_rgba(255,255,255,64);
    u32 CL = color_rgba(0, 255, 255, 255);
    Fmatrix& M = Fidentity;
    // ref_shader				l_tShaderReference = Level().ObjectSpace.dbgGetShader();
    // RCache.set_Shader		(l_tShaderReference);
    (*Level().ObjectSpace.m_pRender)->SetShader();
    //	RCache.dbg_DrawTRI	(M,COP,_F[0],_F[1],CT);
    //	RCache.dbg_DrawTRI	(M,COP,_F[1],_F[2],CT);
    //	RCache.dbg_DrawTRI	(M,COP,_F[2],_F[3],CT);
    //	RCache.dbg_DrawTRI	(M,COP,_F[3],_F[0],CT);
    Level().debug_renderer().draw_line(M, COP, _F[0], CL);
    Level().debug_renderer().draw_line(M, COP, _F[1], CL);
    Level().debug_renderer().draw_line(M, COP, _F[2], CL);
    Level().debug_renderer().draw_line(M, COP, _F[3], CL);

    Level().debug_renderer().draw_line(M, _F[0], _F[1], CL);
    Level().debug_renderer().draw_line(M, _F[1], _F[2], CL);
    Level().debug_renderer().draw_line(M, _F[2], _F[3], CL);
    Level().debug_renderer().draw_line(M, _F[3], _F[0], CL);

    // RCache.set_CullMode			(CULL_CCW);
    GEnv.DRender->CacheSetCullMode(IDebugRender::cmCCW);
    // CHK_DX(HW.pDevice->SetRenderState	(D3DRS_AMBIENT,	0						));
    GEnv.DRender->SetAmbient(0);
}
#endif
