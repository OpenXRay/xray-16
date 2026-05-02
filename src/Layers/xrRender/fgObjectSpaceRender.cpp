#include "stdafx.h"

#ifdef DEBUG

#include "Layers/xrRender/fgObjectSpaceRender.h"
#include "Layers/xrRender/FGDebugDraw.h"

namespace xray::render::fg
{
fgObjectSpaceRender::fgObjectSpaceRender() { m_shDebug.create("debug" DELIMITER "wireframe", "$null"); }
fgObjectSpaceRender::~fgObjectSpaceRender() { m_shDebug.destroy(); }
void fgObjectSpaceRender::Copy(IObjectSpaceRender& _in) { *this = *(fgObjectSpaceRender*)&_in; }
void fgObjectSpaceRender::dbgAddSphere(const Fsphere& sphere, u32 colour) { dbg_S.emplace_back(sphere, colour); }
void fgObjectSpaceRender::dbgReserveSphere(size_t count) { dbg_S.reserve(count); }
void fgObjectSpaceRender::dbgRender()
{
    R_ASSERT(bDebug);

    RCache.set_Shader(m_shDebug);
    for (u32 i = 0; i < q_debug.boxes.size(); i++)
    {
        Fobb& obb = q_debug.boxes[i];
        Fmatrix X, S, R;
        obb.xform_get(X);
        g_debug_draw.DrawOBB(X, obb.m_halfsize, color_xrgb(255, 0, 0));
        S.scale(obb.m_halfsize);
        R.mul(X, S);
        g_debug_draw.DrawEllipse(R, color_xrgb(0, 0, 255));
    }
    q_debug.boxes.clear();

    for (u32 i = 0; i < dbg_S.size(); i++)
    {
        std::pair<Fsphere, u32>& P = dbg_S[i];
        Fsphere& S = P.first;
        Fmatrix M;
        M.scale(S.R, S.R, S.R);
        M.translate_over(S.P);
        g_debug_draw.DrawEllipse(M, P.second);
    }
    dbg_S.clear();
}

void fgObjectSpaceRender::SetShader() { RCache.set_Shader(m_shDebug); }
} // namespace xray::render::fg
#endif // DEBUG
