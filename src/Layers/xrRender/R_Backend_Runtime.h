#ifndef R_BACKEND_RUNTIMEH
#define R_BACKEND_RUNTIMEH
#pragma once

#include "SH_Texture.h"
#include "SH_Matrix.h"
#include "SH_Constant.h"
#include "SH_RT.h"

#ifdef USE_OGL
#include "Layers/xrRenderGL/glR_Backend_Runtime.h"
#include "Layers/xrRenderGL/glState.h"
#elif !defined(USE_DX9)
#include "Layers/xrRenderDX10/dx10R_Backend_Runtime.h"
#include "Layers/xrRenderDX10/StateManager/dx10State.h"
#else // USE_DX9
#include "Layers/xrRenderDX9/dx9R_Backend_Runtime.h"
#endif // USE_OGL

IC void R_xforms::set_c_w(R_constant* C)
{
    c_w = C;
    RCache.set_c(C, m_w);
};
IC void R_xforms::set_c_invw(R_constant* C)
{
    c_invw = C;
    apply_invw();
};
IC void R_xforms::set_c_v(R_constant* C)
{
    c_v = C;
    RCache.set_c(C, m_v);
};
IC void R_xforms::set_c_p(R_constant* C)
{
    c_p = C;
    RCache.set_c(C, m_p);
};
IC void R_xforms::set_c_wv(R_constant* C)
{
    c_wv = C;
    RCache.set_c(C, m_wv);
};
IC void R_xforms::set_c_vp(R_constant* C)
{
    c_vp = C;
    RCache.set_c(C, m_vp);
};
IC void R_xforms::set_c_wvp(R_constant* C)
{
    c_wvp = C;
    RCache.set_c(C, m_wvp);
};

IC void CBackend::set_xform_world(const Fmatrix& M) { xforms.set_W(M); }
IC void CBackend::set_xform_view(const Fmatrix& M) { xforms.set_V(M); }
IC void CBackend::set_xform_project(const Fmatrix& M) { xforms.set_P(M); }
IC const Fmatrix& CBackend::get_xform_world() { return xforms.get_W(); }
IC const Fmatrix& CBackend::get_xform_view() { return xforms.get_V(); }
IC const Fmatrix& CBackend::get_xform_project() { return xforms.get_P(); }
#ifdef USE_OGL
IC GLuint CBackend::get_RT(u32 ID)
#else
IC ID3DRenderTargetView* CBackend::get_RT(u32 ID)
#endif // USE_OGL
{
    VERIFY((ID >= 0) && (ID < 4));

    return pRT[ID];
}

#ifdef USE_OGL
IC GLuint CBackend::get_ZB()
#else
IC ID3DDepthStencilView* CBackend::get_ZB()
#endif // USE_OGL
{
    return pZB;
}
ICF void CBackend::set_States(SState* _state)
{
//	DX10 Manages states using it's own algorithm. Don't mess with it.
#ifdef USE_DX9
    if (state != _state->state)
#endif
    {
        PGO(Msg("PGO:state_block"));
#ifdef DEBUG
        stat.states++;
#endif
        state = _state->state;
        state->Apply();
    }
}

#ifdef _EDITOR
IC void CBackend::set_Matrices(SMatrixList* _M)
{
    if (M != _M)
    {
        M = _M;
        if (M)
        {
            for (u32 it = 0; it < M->size(); it++)
            {
                CMatrix* mat = &*((*M)[it]);
                if (mat && matrices[it] != mat)
                {
                    matrices[it] = mat;
                    mat->Calculate();
                    set_xform(D3DTS_TEXTURE0 + it, mat->xform);
                    //				stat.matrices		++;
                }
            }
        }
    }
}
#endif

IC void CBackend::set_Element(ShaderElement* S, u32 pass)
{
    SPass& P = *(S->passes[pass]);
    set_States(P.state);
    set_PS(P.ps);
    set_VS(P.vs);
#ifdef USE_DX11
    set_GS(P.gs);
    set_HS(P.hs);
    set_DS(P.ds);
    set_CS(P.cs);
#endif
    set_Constants(P.constants);
    set_Textures(P.T);
#ifdef _EDITOR
    set_Matrices(P.M);
#endif
}

ICF void CBackend::set_Shader(Shader* S, u32 pass) { set_Element(S->E[0], pass); }
#endif
