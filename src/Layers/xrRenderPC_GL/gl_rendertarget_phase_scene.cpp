#include "stdafx.h"

// startup
void CRenderTarget::phase_scene_prepare()
{
    PIX_EVENT(phase_scene_prepare);
    // Clear depth & stencil
    //u_setrt	( Device.dwWidth,Device.dwHeight,get_base_rt(),NULL,NULL,get_base_zb() );
    //CHK_DX	( HW.pDevice->Clear	( 0L, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x0, 1.0f, 0L) );
    //	Igor: soft particles

    CEnvDescriptor& E = *g_pGamePersistent->Environment().CurrentEnv;
    float fValue = E.m_fSunShaftsIntensity;
    //	TODO: add multiplication by sun color here
    //if (fValue<0.0001) FlagSunShafts = 0;

    //	TODO: DX10: Check if complete clear of _ALL_ rendertargets will increase
    //	FPS. Make check for SLI configuration.
    if (RImplementation.o.advancedpp &&
        (
            ps_r2_ls_flags.test(R2FLAG_SOFT_PARTICLES | R2FLAG_DOF) ||
            ps_r_sun_shafts > 0 && fValue >= 0.0001 ||
            ps_r_ssao > 0
        )
    )
    {
        //	TODO: DX10: Check if we need to set RT here.
        u_setrt(Device.dwWidth, Device.dwHeight, rt_Position->pRT, 0, 0, rt_MSAADepth->pZRT);

        const Fcolor color{}; // black
        RCache.ClearRT(rt_Position, color);
        // RCache.ClearRT(rt_Normal, color);
        // RCache.ClearRT(rt_Color, color);
        if (!RImplementation.o.dx10_msaa)
            RCache.ClearZB(get_base_zb(), 1.0f, 0);
        else
        {
            RCache.ClearRT(rt_Color, color);
            RCache.ClearRT(rt_Accumulator, color);
            RCache.ClearZB(rt_MSAADepth, 1.0f, 0);
            RCache.ClearZB(get_base_zb(), 1.0f, 0);
        }
    }
    else
    {
        //	TODO: DX10: Check if we need to set RT here.
        u_setrt(Device.dwWidth, Device.dwHeight, get_base_rt(), 0, 0, rt_MSAADepth->pZRT);
        RCache.ClearZB(rt_MSAADepth, 1.0f, 0);
    }

    //	Igor: for volumetric lights
    m_bHasActiveVolumetric = false;
    //	Clear later if try to draw volumetric
}

// begin
void CRenderTarget::phase_scene_begin()
{
    // Targets, use accumulator for temporary storage
    if (!RImplementation.o.dx10_gbuffer_opt)
    {
        if (RImplementation.o.albedo_wo) u_setrt(rt_Position, rt_Normal, rt_Accumulator, rt_MSAADepth->pZRT);
        else u_setrt(rt_Position, rt_Normal, rt_Color, rt_MSAADepth->pZRT);
    }
    else
    {
        if (RImplementation.o.albedo_wo) u_setrt(rt_Position, rt_Accumulator, rt_MSAADepth->pZRT);
        else u_setrt(rt_Position, rt_Color, rt_MSAADepth->pZRT);
        //else								u_setrt		(rt_Position,	rt_Color, rt_Normal,		rt_MSAADepth->pZRT);
    }

    // Stencil - write 0x1 at pixel pos
    RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
                       D3DSTENCILOP_KEEP);

    // Misc		- draw only front-faces
    //	TODO: DX10: siable two-sided stencil here
    //CHK_DX(HW.pDevice->SetRenderState	( D3DRS_TWOSIDEDSTENCILMODE,FALSE				));
    RCache.set_CullMode(CULL_CCW);
    RCache.set_ColorWriteEnable();
}

void CRenderTarget::disable_aniso()
{
    // Disable ANISO
    //	TODO: DX10: disable aniso here
    //for (u32 i=0; i<HW.Caps.raster.dwStages; i++)
    //	CHK_DX(HW.pDevice->SetSamplerState( i, D3DSAMP_MAXANISOTROPY, 1	));
}

// end
void CRenderTarget::phase_scene_end()
{
    disable_aniso();

    if (!RImplementation.o.albedo_wo) return;

    // transfer from "rt_Accumulator" into "rt_Color"
    u_setrt(rt_Color, nullptr, nullptr, rt_MSAADepth->pZRT);
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00); // stencil should be >= 1
    if (RImplementation.o.nvstencil) u_stencil_optimize(SO_Combine);
    RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00); // stencil should be >= 1
    RCache.set_ColorWriteEnable();

    // common calc for quad-rendering
    u32 Offset;
    u32 C = color_rgba(255, 255, 255, 255);
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    Fvector2 p0, p1;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);
    float d_Z = EPS_S, d_W = 1.f;

    // Fill vertex buffer
    FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
    pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
    pv++;
    pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
    pv++;
    pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
    pv++;
    pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
    pv++;
    RCache.Vertex.Unlock(4, g_combine->vb_stride);

    // if (stencil>=1 && aref_pass)	stencil = light_id
    RCache.set_Element(s_accum_mask->E[SE_MASK_ALBEDO]); // masker
    RCache.set_Geometry(g_combine);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}
