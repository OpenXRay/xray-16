#include "stdafx.h"

void CRenderTarget::phase_ssao()
{
    u32 Offset = 0;

    // Clear to zero
    RCache.ClearRT(rt_ssao_temp, {});

    // low/hi RTs
    if (!RImplementation.o.dx10_msaa)
    {
        u_setrt(rt_ssao_temp, 0, 0, 0 /*get_base_zb()*/);
    }
    else
    {
        u_setrt(rt_ssao_temp, 0, 0, 0 /*RImplementation.Target->rt_MSAADepth->pZRT*/);
    }

    RCache.set_Stencil(FALSE);

    /*RCache.set_Stencil					(TRUE,D3DCMP_LESSEQUAL,0x01,0xff,0x00);	// stencil should be >= 1
    if (RImplementation.o.nvstencil)	{
        u_stencil_optimize				(CRenderTarget::SO_Combine);
        RCache.set_ColorWriteEnable		();
    }*/

    // Compute params
    Fmatrix m_v2w;
    m_v2w.invert(Device.mView);

    float fSSAONoise = 2.0f;
    fSSAONoise *= tan(deg2rad(67.5f));
    fSSAONoise /= tan(deg2rad(Device.fFOV));

    float fSSAOKernelSize = 150.0f;
    fSSAOKernelSize *= tan(deg2rad(67.5f));
    fSSAOKernelSize /= tan(deg2rad(Device.fFOV));

    // Fill VB
    float scale_X = float(Device.dwWidth) * 0.5f / float(TEX_jitter);
    float scale_Y = float(Device.dwHeight) * 0.5f / float(TEX_jitter);

    float _w = float(Device.dwWidth) * 0.5f;
    float _h = float(Device.dwHeight) * 0.5f;

    RCache.SetViewport({ 0.f, 0.f, _w, _h, 0.f, 1.f });

    // Fill vertex buffer
    FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
    pv->set(-1, 1, 0, 1, 0, 0, scale_Y);
    pv++;
    pv->set(-1, -1, 0, 0, 0, 0, 0);
    pv++;
    pv->set(1, 1, 1, 1, 0, scale_X, scale_Y);
    pv++;
    pv->set(1, -1, 1, 0, 0, scale_X, 0);
    pv++;
    RCache.Vertex.Unlock(4, g_combine->vb_stride);

    // Draw
    RCache.set_Element(s_ssao->E[0]);
    RCache.set_Geometry(g_combine);

    RCache.set_c("m_v2w", m_v2w);
    RCache.set_c("ssao_noise_tile_factor", fSSAONoise);
    RCache.set_c("ssao_kernel_size", fSSAOKernelSize);
    RCache.set_c("resolution", _w, _h, 1.0f / _w, 1.0f / _h);

    if (!RImplementation.o.dx10_msaa)
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    else
    {
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        /*RCache.set_Stencil( TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0 );
        RCache.Render		( D3DPT_TRIANGLELIST,Offset,0,4,0,2);
        if( RImplementation.o.dx10_msaa_opt )
        {
            RCache.set_Element( s_ssao_msaa[0]->E[0]	);
            RCache.set_Stencil( TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0 );
            RCache.Render	  ( D3DPT_TRIANGLELIST,Offset,0,4,0,2);
        }
        else
        {
            for( u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i )
            {
                RCache.set_Element			( s_ssao_msaa[i]->E[0]	);
                StateManager.SetSampleMask	( u32(1) << i  );
                RCache.set_Stencil			( TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0 );
                RCache.Render				( D3DPT_TRIANGLELIST,Offset,0,4,0,2);
            }
            StateManager.SetSampleMask( 0xffffffff );
        }*/
        // RCache.set_Stencil( FALSE, D3DCMP_EQUAL, 0x01, 0xff, 0 );
    }

    RCache.SetViewport({ 0.f, 0.f, float(Device.dwWidth), float(Device.dwHeight), 0.f, 1.f });

    RCache.set_Stencil(FALSE);
}

void CRenderTarget::phase_downsamp()
{
    // DON'T DO THIS!!!
    // IDirect3DSurface9 *source, *dest;
    // rt_Position->pSurface->GetSurfaceLevel(0, &source);
    // rt_half_depth->pSurface->GetSurfaceLevel(0, &dest);
    // HW.pDevice->StretchRect(source, NULL, dest, NULL, D3DTEXF_POINT);

    // Fvector2	p0,p1;
    u32 Offset = 0;

    u_setrt(rt_half_depth, 0, 0, 0 /*get_base_zb()*/);
    RCache.ClearRT(rt_half_depth, {}); // black
    u32 w = Device.dwWidth;
    u32 h = Device.dwHeight;

    if (RImplementation.o.ssao_half_data)
    {
        RCache.SetViewport({ 0.f, 0.f, float(Device.dwWidth) * 0.5f, float(Device.dwHeight) * 0.5f, 0.f, 1.f });
        w /= 2;
        h /= 2;
    }

    RCache.set_Stencil(FALSE);

    {
        Fmatrix m_v2w;
        m_v2w.invert(Device.mView);

        // Fill VB
        float scale_X = float(w) / float(TEX_jitter);
        float scale_Y = float(h) / float(TEX_jitter);

        // Fill vertex buffer
        FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
        pv->set(-1, 1, 0, 1, 0, 0, scale_Y);
        pv++;
        pv->set(-1, -1, 0, 0, 0, 0, 0);
        pv++;
        pv->set(1, 1, 1, 1, 0, scale_X, scale_Y);
        pv++;
        pv->set(1, -1, 1, 0, 0, scale_X, 0);
        pv++;
        RCache.Vertex.Unlock(4, g_combine->vb_stride);

        // Draw
        RCache.set_Element(s_ssao->E[1]);
        RCache.set_Geometry(g_combine);
        RCache.set_c("m_v2w", m_v2w);

        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }

    if (RImplementation.o.ssao_half_data)
        RCache.SetViewport({ 0.f, 0.f, float(Device.dwWidth), float(Device.dwHeight), 0.f, 1.f });
}
