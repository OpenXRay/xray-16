#ifndef R_BACKEND_RUNTIMEH
#define R_BACKEND_RUNTIMEH
#pragma once

#include "SH_Texture.h"
#include "SH_Matrix.h"
#include "SH_Constant.h"
#include "SH_RT.h"

#include "Layers/xrRender/Debug/dxPixEventWrapper.h"

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/dx11R_Backend_Runtime.h"
#elif defined(USE_OGL)
#include "Layers/xrRenderGL/glR_Backend_Runtime.h"
#include "Layers/xrRenderGL/glState.h"
#endif

IC void R_xforms::set_c_w(R_constant* C)
{
    c_w = C;
    cmd_list.set_c(C, m_w);
};
IC void R_xforms::set_c_invw(R_constant* C)
{
    c_invw = C;
    apply_invw();
};
IC void R_xforms::set_c_v(R_constant* C)
{
    c_v = C;
    cmd_list.set_c(C, m_v);
};
IC void R_xforms::set_c_p(R_constant* C)
{
    c_p = C;
    cmd_list.set_c(C, m_p);
};
IC void R_xforms::set_c_wv(R_constant* C)
{
    c_wv = C;
    cmd_list.set_c(C, m_wv);
};
IC void R_xforms::set_c_vp(R_constant* C)
{
    c_vp = C;
    cmd_list.set_c(C, m_vp);
};
IC void R_xforms::set_c_wvp(R_constant* C)
{
    c_wvp = C;
    cmd_list.set_c(C, m_wvp);
};

IC void CBackend::set_xform_world(const Fmatrix& M) { xforms.set_W(M); }
IC void CBackend::set_xform_view(const Fmatrix& M) { xforms.set_V(M); }
IC void CBackend::set_xform_project(const Fmatrix& M) { xforms.set_P(M); }
IC const Fmatrix& CBackend::get_xform_world() { return xforms.get_W(); }
IC const Fmatrix& CBackend::get_xform_view() { return xforms.get_V(); }
IC const Fmatrix& CBackend::get_xform_project() { return xforms.get_P(); }
#if defined(USE_DX11)
IC ID3DRenderTargetView* CBackend::get_RT(u32 ID)
#elif defined(USE_OGL)
IC GLuint CBackend::get_RT(u32 ID)
#else
#   error No graphics API selected or enabled!
#endif
{
    VERIFY((ID >= 0) && (ID < 4));

    return pRT[ID];
}

#if defined(USE_DX11)
IC ID3DDepthStencilView* CBackend::get_ZB()
#elif defined(USE_OGL)
IC GLuint CBackend::get_ZB()
#else
#   error No graphics API selected or enabled!
#endif
{
    return pZB;
}
ICF void CBackend::set_States(SState* _state)
{
//	DX11 Manages states using it's own algorithm. Don't mess with it.
#ifdef USE_DX9
    if (state != _state->state)
#endif
    {
        PGO(Msg("PGO:state_block"));
        stat.states++;
        state = _state->state;
#if defined(USE_DX11)
        state->Apply(*this);
#else
        state->Apply();
#endif
    }
}

IC void CBackend::set_Matrices(SMatrixList* matrix_list)
{
    if (M != matrix_list)
    {
        M = matrix_list;
        if (M)
        {
            for (u32 it = 0; it < M->size(); it++)
            {
                CMatrix* mat = (*M)[it]._get();
                if (mat && matrices[it] != mat)
                {
                    matrices[it] = mat;
                    mat->Calculate();
                    set_xform(D3DTS_TEXTURE0 + it, mat->xform);
                    stat.matrices++;
                }
            }
        }
    }
}

IC void CBackend::set_Pass(SPass* P)
{
    set_States(P->state);
#ifdef USE_OGL
    if (P->pp)
        set_PP(P->pp);
    else
#endif
    {
        set_PS(P->ps);
        set_VS(P->vs);
#ifdef USE_DX11
        set_GS(P->gs);
        set_HS(P->hs);
        set_DS(P->ds);
        set_CS(P->cs);
#endif
    }
    set_Constants(P->constants);
    set_Textures(P->T);
    set_Matrices(P->M);
}

ICF void CBackend::set_Element(ShaderElement* S, u32 pass) { set_Pass(S->passes[pass]); }
ICF void CBackend::set_Shader(Shader* S, u32 pass) { set_Element(S->E[0], pass); }
#endif
