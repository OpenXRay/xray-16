#include "stdafx.h"

void CRenderTarget::create_minmax_SM(CBackend& cmd_list)
{
    u32 Offset;
    float d_Z = EPS_S, d_W = 1.f;
    u32 C = color_rgba(255, 255, 255, 255);

    // Fill vertex buffer
    FVF::TL2uv* pv = (FVF::TL2uv*)RImplementation.Vertex.Lock(4, g_combine_2UV->vb_stride, Offset);
    pv->set(-1, -1, 0, d_W, C, 0, 1, 0, 0);
    pv++;
    pv->set(-1, 1, d_Z, d_W, C, 0, 0, 0, 0);
    pv++;
    pv->set(1, -1, d_Z, d_W, C, 1, 1, 0, 0);
    pv++;
    pv->set(1, 1, d_Z, d_W, C, 1, 0, 0, 0);
    pv++;
    RImplementation.Vertex.Unlock(4, g_combine_2UV->vb_stride);
    //u_setrt	(rt_smap_depth_minmax_temp,NULL,NULL,NULL);
    u_setrt(cmd_list, rt_smap_depth_minmax, 0, 0, 0);
    cmd_list.set_Element(s_create_minmax_sm->E[0]);
    cmd_list.set_Geometry(g_combine_2UV);
    cmd_list.set_Stencil(FALSE, D3DCMP_ALWAYS, 0x80, 0xFF, 0x80,
        D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
    cmd_list.set_ColorWriteEnable();
    cmd_list.set_ZFunc(D3DCMP_ALWAYS);
    cmd_list.set_Z(FALSE);
    cmd_list.set_CullMode(D3DCULL_NONE);
    cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

    cmd_list.set_ColorWriteEnable();
}
