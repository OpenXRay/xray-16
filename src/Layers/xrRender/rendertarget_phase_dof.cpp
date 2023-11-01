#include "stdafx.h"

void CRenderTarget::phase_dof()
{
    //Constants
    u32 Offset = 0;
    u32 C = color_rgba(0, 0, 0, 255);

    float d_Z = EPS_S;
    float d_W = 1.0f;
    float w = float(Device.dwWidth);
    float h = float(Device.dwHeight);

    Fvector2 p0, p1;
    p0.set(0.0f, 0.0f);
    p1.set(1.0f, 1.0f);

    //DoF vectors
    Fvector2 vDofKernel;
    vDofKernel.set(0.5f / Device.dwWidth, 0.5f / Device.dwHeight);
    vDofKernel.mul(ps_r2_dof_kernel_size);
    Fvector3 dof;
    g_pGamePersistent->GetCurrentDof(dof);

    //////////////////////////////////////////////////////////////////////////
    //Set MSAA/NonMSAA rendertarget
    u_setrt(RCache, rt_dof, 0, 0, get_base_zb());

    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    //Fill vertex buffer
    FVF::TL* pv = (FVF::TL*)RImplementation.Vertex.Lock(4, g_combine->vb_stride, Offset);
    pv->set(0, float(h), d_Z, d_W, C, p0.x, p1.y); pv++;
    pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y); pv++;
    pv->set(float(w), float(h), d_Z, d_W, C, p1.x, p1.y); pv++;
    pv->set(float(w), 0, d_Z, d_W, C, p1.x, p0.y); pv++;
    RImplementation.Vertex.Unlock(4, g_combine->vb_stride);

    //Set pass
    RCache.set_Element(s_dof->E[0]);

    //Set paramterers
    //RCache.set_c("taa_params", ps_taa_params.x, ps_taa_params.y, 0, 0);
    RCache.set_c("dof_params", dof.x, dof.y, dof.z, ps_r2_dof_sky);
    RCache.set_c("dof_kernel", vDofKernel.x, vDofKernel.y, ps_r2_dof_kernel_size, 0.f);

    //Set geometry
    RCache.set_Geometry(g_combine);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    HW.get_context(CHW::IMM_CTX_ID)->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_dof->pTexture->surface_get());
};
