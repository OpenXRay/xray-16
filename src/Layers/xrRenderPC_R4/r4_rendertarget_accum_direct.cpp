#include "stdafx.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
static Fvector3 corners[8] =
{
    { -1, -1, 0.7f }, { -1, -1, +1   },
    { -1, +1, +1   }, { -1, +1, 0.7f },
    { +1, +1, +1   }, { +1, +1, 0.7f },
    { +1, -1, +1   }, { +1, -1, 0.7f }
};
static u16 facetable[16][3] =
{
    { 3, 2, 1 },
    { 3, 1, 0 },
    { 7, 6, 5 },
    { 5, 6, 4 },
    { 3, 5, 2 },
    { 4, 2, 5 },
    { 1, 6, 7 },
    { 7, 0, 1 },

    { 5, 3, 0 },
    { 7, 5, 0 },

    { 1, 4, 6 },
    { 2, 4, 1 },
};
void CRenderTarget::accum_direct(u32 sub_phase)
{
    // Choose normal code-path or filtered
    phase_accumulator();
    if (RImplementation.o.sunfilter)
    {
        accum_direct_f(sub_phase);
        return;
    }

    //	choose corect element for the sun shader
    u32 uiElementIndex = sub_phase;
    if ((uiElementIndex == SE_SUN_NEAR) && use_minmax_sm_this_frame())
        uiElementIndex = SE_SUN_NEAR_MINMAX;

    //	TODO: DX10: Remove half pixe offset
    // *** assume accumulator setted up ***
    light* fuckingsun = (light*)RImplementation.Lights.sun._get();

    // Common calc for quad-rendering
    u32 Offset;
    u32 C = color_rgba(255, 255, 255, 255);
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    Fvector2 p0, p1;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);
    float d_Z = EPS_S, d_W = 1.f;

    // Common constants (light-related)
    Fvector L_dir, L_clr;
    float L_spec;
    L_clr.set(fuckingsun->color.r, fuckingsun->color.g, fuckingsun->color.b);
    L_spec = u_diffuse2s(L_clr);
    Device.mView.transform_dir(L_dir, fuckingsun->direction);
    L_dir.normalize();

    // Perform masking (only once - on the first/near phase)
    RCache.set_CullMode(CULL_NONE);
    PIX_EVENT(SE_SUN_NEAR_sub_phase);
    if (SE_SUN_NEAR == sub_phase) //.
    // if( 0 )
    {
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
        RCache.set_Geometry(g_combine);

        // setup
        float intensity = 0.3f * fuckingsun->color.r + 0.48f * fuckingsun->color.g + 0.22f * fuckingsun->color.b;
        Fvector dir = L_dir;
        dir.normalize().mul(-_sqrt(intensity + EPS));
        RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]); // masker
        RCache.set_c("Ldynamic_dir", dir.x, dir.y, dir.z, 0.f);

        // if (stencil>=1 && aref_pass)	stencil = light_id
        //	Done in blender!
        // RCache.set_ColorWriteEnable	(FALSE		);
        if (!RImplementation.o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP,
                D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            // per pixel rendering // checked Holger
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0x81, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
                D3DSTENCILOP_KEEP);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

            // per sample rendering
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_accum_mask_msaa[0]->E[SE_MASK_DIRECT]); // masker
                RCache.set_CullMode(CULL_NONE);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0x81, 0x7f, D3DSTENCILOP_KEEP,
                    D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            else
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_accum_mask_msaa[i]->E[SE_MASK_DIRECT]); // masker
                    RCache.set_CullMode(CULL_NONE);
                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0x81, 0x7f, D3DSTENCILOP_KEEP,
                        D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP,
                D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        }
    }

    // recalculate d_Z, to perform depth-clipping
    Fvector center_pt;
    center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, ps_r2_sun_near);
    Device.mFullTransform.transform(center_pt);
    d_Z = center_pt.z;

    // nv-stencil recompression
    if (RImplementation.o.nvstencil && (SE_SUN_NEAR == sub_phase))
        u_stencil_optimize(); //. driver bug?

    PIX_EVENT(Perform_lighting);

    // Perform lighting
    {
        phase_accumulator();
        RCache.set_CullMode(CULL_NONE);
        RCache.set_ColorWriteEnable();

        // texture adjustment matrix
        // float			fTexelOffs			= (.5f / float(RImplementation.o.smapsize));
        // float			fRange				=
        // (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
        // float			fBias				=
        // (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
        // Fmatrix			m_TexelAdjust		=
        //{
        //	0.5f,				0.0f,				0.0f,			0.0f,
        //	0.0f,				-0.5f,				0.0f,			0.0f,
        //	0.0f,				0.0f,				fRange,			0.0f,
        //	0.5f + fTexelOffs,	0.5f + fTexelOffs,	fBias,			1.0f
        //};
        float fRange = (SE_SUN_NEAR == sub_phase) ? ps_r2_sun_depth_near_scale : ps_r2_sun_depth_far_scale;
        // float			fBias				=
        // (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
        //	TODO: DX10: Remove this when fix inverse culling for far region
        float fBias = (SE_SUN_NEAR == sub_phase) ? (-ps_r2_sun_depth_near_bias) : ps_r2_sun_depth_far_bias;
        Fmatrix m_TexelAdjust = {
            0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, fRange, 0.0f, 0.5f, 0.5f, fBias, 1.0f};

        // compute xforms
        FPU::m64r();
        Fmatrix xf_invview;
        xf_invview.invert(Device.mView);

        // shadow xform
        Fmatrix m_shadow;
        {
            Fmatrix xf_project;
            xf_project.mul(m_TexelAdjust, fuckingsun->X.D.combine);
            m_shadow.mul(xf_project, xf_invview);

            // tsm-bias
            if ((SE_SUN_FAR == sub_phase) && (RImplementation.o.HW_smap))
            {
                Fvector bias;
                bias.mul(L_dir, ps_r2_sun_tsm_bias);
                Fmatrix bias_t;
                bias_t.translate(bias);
                m_shadow.mulB_44(bias_t);
            }
            FPU::m24r();
        }

        // clouds xform
        Fmatrix m_clouds_shadow;
        {
            static float w_shift = 0;
            Fmatrix m_xform;
            Fvector direction = fuckingsun->direction;
            float w_dir = g_pGamePersistent->Environment().CurrentEnv->wind_direction;
            // float	w_speed				= g_pGamePersistent->Environment().CurrentEnv->wind_velocity	;
            Fvector normal;
            normal.setHP(w_dir, 0);
            w_shift += 0.003f * Device.fTimeDelta;
            Fvector position;
            position.set(0, 0, 0);
            m_xform.build_camera_dir(position, direction, normal);
            Fvector localnormal;
            m_xform.transform_dir(localnormal, normal);
            localnormal.normalize();
            m_clouds_shadow.mul(m_xform, xf_invview);
            m_xform.scale(0.002f, 0.002f, 1.f);
            m_clouds_shadow.mulA_44(m_xform);
            m_xform.translate(localnormal.mul(w_shift));
            m_clouds_shadow.mulA_44(m_xform);
        }

        // Make jitter texture
        Fvector2 j0, j1;
        float scale_X = float(Device.dwWidth) / float(TEX_jitter);
        // float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
        float offset = (.5f / float(TEX_jitter));
        j0.set(offset, offset);
        j1.set(scale_X, scale_X).add(offset);

        // Fill vertex buffer
        FVF::TL2uv* pv = (FVF::TL2uv*)RCache.Vertex.Lock(4, g_combine_2UV->vb_stride, Offset);
        // pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y, j0.x, j1.y);
        // pv++;
        // pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y, j0.x, j0.y);
        // pv++;
        // pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y, j1.x, j1.y);
        // pv++;
        // pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y, j1.x, j0.y);
        // pv++;
        pv->set(-1, -1, d_Z, d_W, C, 0, 1, 0, scale_X);
        pv++;
        pv->set(-1, 1, d_Z, d_W, C, 0, 0, 0, 0);
        pv++;
        pv->set(1, -1, d_Z, d_W, C, 1, 1, scale_X, scale_X);
        pv++;
        pv->set(1, 1, d_Z, d_W, C, 1, 0, scale_X, 0);
        pv++;
        RCache.Vertex.Unlock(4, g_combine_2UV->vb_stride);
        RCache.set_Geometry(g_combine_2UV);

        // setup
        RCache.set_Element(s_accum_direct->E[uiElementIndex]);
        RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0.f);
        RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
        RCache.set_c("m_shadow", m_shadow);
        RCache.set_c("m_sunmask", m_clouds_shadow);

        // nv-DBT
        float zMin, zMax;
        if (SE_SUN_NEAR == sub_phase)
        {
            zMin = 0;
            zMax = ps_r2_sun_near;
        }
        else
        {
            extern float OLES_SUN_LIMIT_27_01_07;
            zMin = ps_r2_sun_near;
            zMax = OLES_SUN_LIMIT_27_01_07;
        }
        center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMin);
        Device.mFullTransform.transform(center_pt);
        zMin = center_pt.z;

        center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMax);
        Device.mFullTransform.transform(center_pt);
        zMax = center_pt.z;

        //	TODO: DX10: Check if DX10 has analog for NV DBT
        //		if (u_DBT_enable(zMin,zMax))	{
        // z-test always
        //			RCache.set_ZFunc(D3DCMP_ALWAYS);
        //			HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        //		}

        // Fetch4 : enable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
        //		}

        // setup stencil
        if (!RImplementation.o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            // per pixel
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

            // per sample
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_accum_direct_msaa[0]->E[uiElementIndex]);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                RCache.set_CullMode(CULL_NONE);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            else
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_accum_direct_msaa[i]->E[uiElementIndex]);
                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                    RCache.set_CullMode(CULL_NONE);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
        }

        // Fetch4 : disable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET1  MAKEFOURCC('G','E','T','1')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
        //		}

        //	TODO: DX10: Check if DX10 has analog for NV DBT
        // disable depth bounds
        //		u_DBT_disable	();

        //	Igor: draw volumetric here
        // if (ps_r2_ls_flags.test(R2FLAG_SUN_SHAFTS))
        if (RImplementation.o.advancedpp && (ps_r_sun_shafts > 0))
            accum_direct_volumetric(sub_phase, Offset, m_shadow);
    }
}

void CRenderTarget::accum_direct_cascade(u32 sub_phase, Fmatrix& xform, Fmatrix& xform_prev, float fBias)
{
    // Choose normal code-path or filtered
    phase_accumulator();
    if (RImplementation.o.sunfilter)
    {
        accum_direct_f(sub_phase);
        return;
    }

    //	choose correct element for the sun shader
    u32 uiElementIndex = sub_phase;
    if ((uiElementIndex == SE_SUN_NEAR) && use_minmax_sm_this_frame())
        uiElementIndex = SE_SUN_NEAR_MINMAX;

    //	TODO: DX10: Remove half pixe offset
    // *** assume accumulator setted up ***
    light* fuckingsun = (light*)RImplementation.Lights.sun._get();

    // Common calc for quad-rendering
    u32 Offset;
    u32 C = color_rgba(255, 255, 255, 255);
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    Fvector2 p0, p1;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);
    float d_Z = EPS_S, d_W = 1.f;

    // Common constants (light-related)
    Fvector L_dir, L_clr;
    float L_spec;
    L_clr.set(fuckingsun->color.r, fuckingsun->color.g, fuckingsun->color.b);
    L_spec = u_diffuse2s(L_clr);
    Device.mView.transform_dir(L_dir, fuckingsun->direction);
    L_dir.normalize();

    // Perform masking (only once - on the first/near phase)
    RCache.set_CullMode(CULL_NONE);
    PIX_EVENT(SE_SUN_NEAR_sub_phase);
    if (SE_SUN_NEAR == sub_phase) //.
    // if( 0 )
    {
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
        RCache.set_Geometry(g_combine);

        // setup
        float intensity = 0.3f * fuckingsun->color.r + 0.48f * fuckingsun->color.g + 0.22f * fuckingsun->color.b;
        Fvector dir = L_dir;
        dir.normalize().mul(-_sqrt(intensity + EPS));
        RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]); // masker
        RCache.set_c("Ldynamic_dir", dir.x, dir.y, dir.z, 0.f);

        // if (stencil>=1 && aref_pass)	stencil = light_id
        //	Done in blender!
        // RCache.set_ColorWriteEnable	(FALSE		);
        if (!RImplementation.o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP,
                D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            // per pixel rendering // checked Holger
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0x81, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
                D3DSTENCILOP_KEEP);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

            // per sample rendering
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_accum_mask_msaa[0]->E[SE_MASK_DIRECT]); // masker
                RCache.set_CullMode(CULL_NONE);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0x81, 0x7f, D3DSTENCILOP_KEEP,
                    D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            else
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_accum_mask_msaa[i]->E[SE_MASK_DIRECT]); // masker
                    RCache.set_CullMode(CULL_NONE);
                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0x81, 0x7f, D3DSTENCILOP_KEEP,
                        D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP,
                D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        }
    }

    // recalculate d_Z, to perform depth-clipping
    Fvector center_pt;
    center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, ps_r2_sun_near);
    Device.mFullTransform.transform(center_pt);
    d_Z = center_pt.z;

    // nv-stencil recompression
    if (RImplementation.o.nvstencil && (SE_SUN_NEAR == sub_phase))
        u_stencil_optimize(); //. driver bug?

    PIX_EVENT(Perform_lighting);

    // Perform lighting
    {
        phase_accumulator();
        RCache.set_CullMode(CULL_CCW); //******************************************************************
        RCache.set_ColorWriteEnable();

        // texture adjustment matrix
        // float			fTexelOffs			= (.5f / float(RImplementation.o.smapsize));
        // float			fRange				=
        // (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
        // float			fBias				=
        // (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
        // Fmatrix			m_TexelAdjust		=
        //{
        //	0.5f,				0.0f,				0.0f,			0.0f,
        //	0.0f,				-0.5f,				0.0f,			0.0f,
        //	0.0f,				0.0f,				fRange,			0.0f,
        //	0.5f + fTexelOffs,	0.5f + fTexelOffs,	fBias,			1.0f
        //};
        float fRange = (SE_SUN_NEAR == sub_phase) ? ps_r2_sun_depth_near_scale : ps_r2_sun_depth_far_scale;
        // float			fBias				=
        // (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
        //	TODO: DX10: Remove this when fix inverse culling for far region
        //		float			fBias				=
        //(SE_SUN_NEAR==sub_phase)?(-ps_r2_sun_depth_near_bias):ps_r2_sun_depth_far_bias;
        Fmatrix m_TexelAdjust = {
            0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, fRange, 0.0f, 0.5f, 0.5f, fBias, 1.0f};

        // compute xforms
        FPU::m64r();
        Fmatrix xf_invview;
        xf_invview.invert(Device.mView);

        // shadow xform
        Fmatrix m_shadow;
        {
            Fmatrix xf_project;
            xf_project.mul(m_TexelAdjust, fuckingsun->X.D.combine);
            m_shadow.mul(xf_project, xf_invview);

            // tsm-bias
            if ((SE_SUN_FAR == sub_phase) && (RImplementation.o.HW_smap))
            {
                Fvector bias;
                bias.mul(L_dir, ps_r2_sun_tsm_bias);
                Fmatrix bias_t;
                bias_t.translate(bias);
                m_shadow.mulB_44(bias_t);
            }
            FPU::m24r();
        }

        // clouds xform
        Fmatrix m_clouds_shadow;
        {
            static float w_shift = 0;
            Fmatrix m_xform;
            Fvector direction = fuckingsun->direction;
            float w_dir = g_pGamePersistent->Environment().CurrentEnv->wind_direction;
            // float	w_speed				= g_pGamePersistent->Environment().CurrentEnv->wind_velocity	;
            Fvector normal;
            normal.setHP(w_dir, 0);
            w_shift += 0.003f * Device.fTimeDelta;
            Fvector position;
            position.set(0, 0, 0);
            m_xform.build_camera_dir(position, direction, normal);
            Fvector localnormal;
            m_xform.transform_dir(localnormal, normal);
            localnormal.normalize();
            m_clouds_shadow.mul(m_xform, xf_invview);
            m_xform.scale(0.002f, 0.002f, 1.f);
            m_clouds_shadow.mulA_44(m_xform);
            m_xform.translate(localnormal.mul(w_shift));
            m_clouds_shadow.mulA_44(m_xform);
        }

        // Compute textgen texture for pixel shader, for possitions texture.
        Fmatrix m_Texgen;
        m_Texgen.identity();
        RCache.xforms.set_W(m_Texgen);
        RCache.xforms.set_V(Device.mView);
        RCache.xforms.set_P(Device.mProject);
        u_compute_texgen_screen(m_Texgen);

        // Make jitter texture
        Fvector2 j0, j1;
        float scale_X = float(Device.dwWidth) / float(TEX_jitter);
        // float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
        float offset = (.5f / float(TEX_jitter));
        j0.set(offset, offset);
        j1.set(scale_X, scale_X).add(offset);

        // Fill vertex buffer
        u32 i_offset;
        {
            u16* pib = RCache.Index.Lock(sizeof(facetable) / sizeof(u16), i_offset);
            CopyMemory(pib, &facetable, sizeof(facetable));
            RCache.Index.Unlock(sizeof(facetable) / sizeof(u16));

            // corners

            u32 ver_count = sizeof(corners) / sizeof(Fvector3);
            Fvector4* pv = (Fvector4*)RCache.Vertex.Lock(ver_count, g_combine_cuboid.stride(), Offset);

            Fmatrix inv_XDcombine;
            if (/*ps_r2_ls_flags_ext.is(R2FLAGEXT_SUN_ZCULLING) &&*/ sub_phase == SE_SUN_FAR)
                inv_XDcombine.invert(xform_prev);
            else
                inv_XDcombine.invert(xform);

            for (u32 i = 0; i < ver_count; ++i)
            {
                Fvector3 tmp_vec;
                inv_XDcombine.transform(tmp_vec, corners[i]);
                pv->set(tmp_vec.x, tmp_vec.y, tmp_vec.z, 1);
                pv++;
            }
            RCache.Vertex.Unlock(ver_count, g_combine_cuboid.stride());
        }

        RCache.set_Geometry(g_combine_cuboid);

        // setup
        RCache.set_Element(s_accum_direct->E[uiElementIndex]);
        RCache.set_c("m_texgen", m_Texgen);
        RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0.f);
        RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
        RCache.set_c("m_shadow", m_shadow);
        RCache.set_c("m_sunmask", m_clouds_shadow);

        // Pass view vector projected in shadow space to far pixel shader
        // Needed for shadow fading.
        if (sub_phase == SE_SUN_FAR)
        {
            Fvector3 view_viewspace;
            view_viewspace.set(0, 0, 1);

            m_shadow.transform_dir(view_viewspace);
            Fvector4 view_projlightspace;
            view_projlightspace.set(view_viewspace.x, view_viewspace.y, 0, 0);
            view_projlightspace.normalize();

            RCache.set_c("view_shadow_proj", view_projlightspace);
        }

        // nv-DBT
        float zMin, zMax;
        if (SE_SUN_NEAR == sub_phase)
        {
            zMin = 0;
            zMax = ps_r2_sun_near;
        }
        else
        {
            extern float OLES_SUN_LIMIT_27_01_07;
            zMin = ps_r2_sun_near;
            zMax = OLES_SUN_LIMIT_27_01_07;
        }
        center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMin);
        Device.mFullTransform.transform(center_pt);
        zMin = center_pt.z;

        center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMax);
        Device.mFullTransform.transform(center_pt);
        zMax = center_pt.z;

        //	TODO: DX10: Check if DX10 has analog for NV DBT
        //		if (u_DBT_enable(zMin,zMax))	{
        // z-test always
        //			RCache.set_ZFunc(D3DCMP_ALWAYS);
        //			HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        //		}

        // Fetch4 : enable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
        //		}

        // Enable Z function only for near and middle cascades, the far one is restricted by only stencil.
        if ((SE_SUN_NEAR == sub_phase || SE_SUN_MIDDLE == sub_phase))
            RCache.set_ZFunc(D3DCMP_GREATEREQUAL);
        else if (!ps_r2_ls_flags_ext.is(R2FLAGEXT_SUN_ZCULLING))
            RCache.set_ZFunc(D3DCMP_ALWAYS);
        else
            RCache.set_ZFunc(D3DCMP_LESS);

        u32 st_mask = 0xFE;
        _D3DSTENCILOP st_pass = D3DSTENCILOP_ZERO;

        if (sub_phase == SE_SUN_FAR)
        {
            st_mask = 0x00;
            st_pass = D3DSTENCILOP_KEEP;
        }

        // setup stencil
        if (!RImplementation.o.dx10_msaa)
        {
            // RCache.set_Stencil	(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
            RCache.set_Stencil(
                TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, st_mask, D3DSTENCILOP_KEEP, st_pass, D3DSTENCILOP_KEEP);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
        }
        else
        {
            // per pixel
            // RCache.set_Stencil	(TRUE,D3DCMP_EQUAL,dwLightMarkerID,0xff,0x00);
            RCache.set_Stencil(
                TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, st_mask, D3DSTENCILOP_KEEP, st_pass, D3DSTENCILOP_KEEP);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);

            // per sample
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_accum_direct_msaa[0]->E[uiElementIndex]);

                if ((SE_SUN_NEAR == sub_phase || SE_SUN_MIDDLE == sub_phase))
                    RCache.set_ZFunc(D3DCMP_GREATEREQUAL);
                else if (!ps_r2_ls_flags_ext.is(R2FLAGEXT_SUN_ZCULLING))
                    RCache.set_ZFunc(D3DCMP_ALWAYS);
                else
                    RCache.set_ZFunc(D3DCMP_LESS);

                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, st_mask, D3DSTENCILOP_KEEP,
                    st_pass, D3DSTENCILOP_KEEP);
                RCache.set_CullMode(CULL_NONE);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
            }
            else
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_accum_direct_msaa[i]->E[uiElementIndex]);

                    if ((SE_SUN_NEAR == sub_phase || SE_SUN_MIDDLE == sub_phase))
                        RCache.set_ZFunc(D3DCMP_GREATEREQUAL);
                    else if (!ps_r2_ls_flags_ext.is(R2FLAGEXT_SUN_ZCULLING))
                        RCache.set_ZFunc(D3DCMP_ALWAYS);
                    else
                        RCache.set_ZFunc(D3DCMP_LESS);

                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, st_mask, D3DSTENCILOP_KEEP,
                        st_pass, D3DSTENCILOP_KEEP);
                    RCache.set_CullMode(CULL_NONE);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
        }

        // Fetch4 : disable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET1  MAKEFOURCC('G','E','T','1')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
        //		}

        //	TODO: DX10: Check if DX10 has analog for NV DBT
        // disable depth bounds
        //		u_DBT_disable	();

        //	Igor: draw volumetric here
        // if (ps_r2_ls_flags.test(R2FLAG_SUN_SHAFTS))
        if (RImplementation.o.advancedpp && (ps_r_sun_shafts > 0) && sub_phase == SE_SUN_FAR)
            accum_direct_volumetric(sub_phase, Offset, m_shadow);
    }
}

void CRenderTarget::accum_direct_blend()
{
    PIX_EVENT(accum_direct_blend);
    // blend-copy
    if (!RImplementation.o.fp16_blend)
    {
        u_setrt(rt_Accumulator, NULL, NULL, rt_MSAADepth->pZRT);

        //	TODO: DX10: remove half pixel offset
        // Common calc for quad-rendering
        u32 Offset;
        u32 C = color_rgba(255, 255, 255, 255);
        float _w = float(Device.dwWidth);
        float _h = float(Device.dwHeight);
        Fvector2 p0, p1;
        p0.set(.5f / _w, .5f / _h);
        p1.set((_w + .5f) / _w, (_h + .5f) / _h);
        float d_Z = EPS_S, d_W = 1.f;

        // Fill vertex buffer
        FVF::TL2uv* pv = (FVF::TL2uv*)RCache.Vertex.Lock(4, g_combine_2UV->vb_stride, Offset);
        // pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y, p0.x, p1.y);
        // pv++;
        // pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y, p0.x, p0.y);
        // pv++;
        // pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y, p1.x, p1.y);
        // pv++;
        // pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y, p1.x, p0.y);
        // pv++;
        pv->set(-1, -1, d_Z, d_W, C, 0, 1, 0, 1);
        pv++;
        pv->set(-1, 1, d_Z, d_W, C, 0, 0, 0, 0);
        pv++;
        pv->set(1, -1, d_Z, d_W, C, 1, 1, 1, 1);
        pv++;
        pv->set(1, 1, d_Z, d_W, C, 1, 0, 1, 0);
        pv++;
        RCache.Vertex.Unlock(4, g_combine_2UV->vb_stride);
        RCache.set_Geometry(g_combine_2UV);
        RCache.set_Element(s_accum_mask->E[SE_MASK_ACCUM_2D]);
        if (!RImplementation.o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            // per pixel
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

            // per sample
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_accum_mask_msaa[0]->E[SE_MASK_ACCUM_2D]);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            else // checked Holger
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_accum_mask_msaa[i]->E[SE_MASK_ACCUM_2D]);
                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
        }
    }
    // dwLightMarkerID				+= 2;
    increment_light_marker();
}

void CRenderTarget::accum_direct_f(u32 sub_phase)
{
    PIX_EVENT(accum_direct_f);
    // Select target
    if (SE_SUN_LUMINANCE == sub_phase)
    {
        accum_direct_lum();
        return;
    }
    phase_accumulator();
    u_setrt(rt_Generic_0_r, NULL, NULL, rt_MSAADepth->pZRT);

    // *** assume accumulator setted up ***
    light* fuckingsun = (light*)RImplementation.Lights.sun._get();

    // Common calc for quad-rendering
    u32 Offset;
    u32 C = color_rgba(255, 255, 255, 255);
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    Fvector2 p0, p1;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);
    float d_Z = EPS_S, d_W = 1.f;

    // Common constants (light-related)
    Fvector L_dir, L_clr;
    float L_spec;
    L_clr.set(fuckingsun->color.r, fuckingsun->color.g, fuckingsun->color.b);
    L_spec = u_diffuse2s(L_clr);
    Device.mView.transform_dir(L_dir, fuckingsun->direction);
    L_dir.normalize();

    // Perform masking (only once - on the first/near phase)
    RCache.set_CullMode(CULL_NONE);
    if (SE_SUN_NEAR == sub_phase) //.
    {
        // For sun-filter - clear to zero
        RCache.ClearRT(rt_Generic_0, {});

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
        RCache.set_Geometry(g_combine);

        // setup
        float intensity = 0.3f * fuckingsun->color.r + 0.48f * fuckingsun->color.g + 0.22f * fuckingsun->color.b;
        Fvector dir = L_dir;
        dir.normalize().mul(-_sqrt(intensity + EPS));
        RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]); // masker
        RCache.set_c("Ldynamic_dir", dir.x, dir.y, dir.z, 0.f);

        // if (stencil>=1 && aref_pass)	stencil = light_id
        //	Done in blender!
        // RCache.set_ColorWriteEnable	(FALSE		);
        if (!RImplementation.o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP,
                D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            // per pixel
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0x81, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
                D3DSTENCILOP_KEEP);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

            // per sample
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_accum_mask_msaa[0]->E[SE_MASK_DIRECT]); // masker
                RCache.set_Stencil(TRUE, D3DCMP_LESS, dwLightMarkerID, 0x81, 0x7f, D3DSTENCILOP_KEEP,
                    D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
                RCache.set_CullMode(CULL_NONE);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            else
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_accum_mask_msaa[i]->E[SE_MASK_DIRECT]); // masker
                    RCache.set_Stencil(TRUE, D3DCMP_LESS, dwLightMarkerID, 0x81, 0x7f, D3DSTENCILOP_KEEP,
                        D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
                    RCache.set_CullMode(CULL_NONE);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP,
                D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        }
    }

    // recalculate d_Z, to perform depth-clipping
    Fvector center_pt;
    center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, ps_r2_sun_near);
    Device.mFullTransform.transform(center_pt);
    d_Z = center_pt.z;

    // nv-stencil recompression
    if (RImplementation.o.nvstencil && (SE_SUN_NEAR == sub_phase))
        u_stencil_optimize(); //. driver bug?

    // Perform lighting
    {
        u_setrt(rt_Generic_0_r, NULL, NULL, rt_MSAADepth->pZRT); // ensure RT is set
        RCache.set_CullMode(CULL_NONE);
        RCache.set_ColorWriteEnable();

        // texture adjustment matrix
        float fTexelOffs = (.5f / float(RImplementation.o.smapsize));
        float fRange = (SE_SUN_NEAR == sub_phase) ? ps_r2_sun_depth_near_scale : ps_r2_sun_depth_far_scale;
        // float			fBias				=
        // (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
        //	TODO: DX10: Remove this when fix inverse culling for far region
        float fBias = (SE_SUN_NEAR == sub_phase) ? ps_r2_sun_depth_near_bias : -ps_r2_sun_depth_far_bias;
        Fmatrix m_TexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, fRange, 0.0f,
            0.5f + fTexelOffs, 0.5f + fTexelOffs, fBias, 1.0f};

        // compute xforms
        Fmatrix m_shadow;
        {
            FPU::m64r();
            Fmatrix xf_invview;
            xf_invview.invert(Device.mView);
            Fmatrix xf_project;
            xf_project.mul(m_TexelAdjust, fuckingsun->X.D.combine);
            m_shadow.mul(xf_project, xf_invview);

            // tsm-bias
            if (SE_SUN_FAR == sub_phase)
            {
                Fvector bias;
                bias.mul(L_dir, ps_r2_sun_tsm_bias);
                Fmatrix bias_t;
                bias_t.translate(bias);
                m_shadow.mulB_44(bias_t);
            }
            FPU::m24r();
        }

        // Make jitter texture
        Fvector2 j0, j1;
        float scale_X = float(Device.dwWidth) / float(TEX_jitter);
        // float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
        float offset = (.5f / float(TEX_jitter));
        j0.set(offset, offset);
        j1.set(scale_X, scale_X).add(offset);

        // Fill vertex buffer
        FVF::TL2uv* pv = (FVF::TL2uv*)RCache.Vertex.Lock(4, g_combine_2UV->vb_stride, Offset);
        // pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y, j0.x, j1.y);
        // pv++;
        // pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y, j0.x, j0.y);
        // pv++;
        // pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y, j1.x, j1.y);
        // pv++;
        // pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y, j1.x, j0.y);
        // pv++;
        pv->set(-1, -1, d_Z, d_W, C, 0, 1, 0, scale_X);
        pv++;
        pv->set(-1, 1, d_Z, d_W, C, 0, 0, 0, 0);
        pv++;
        pv->set(1, -1, d_Z, d_W, C, 1, 1, scale_X, scale_X);
        pv++;
        pv->set(1, 1, d_Z, d_W, C, 1, 0, scale_X, 0);
        pv++;
        RCache.Vertex.Unlock(4, g_combine_2UV->vb_stride);
        RCache.set_Geometry(g_combine_2UV);

        // setup
        RCache.set_Element(s_accum_direct->E[sub_phase]);
        RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0.f);
        RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
        RCache.set_c("m_shadow", m_shadow);

        if (!RImplementation.o.dx10_msaa)
        {
            // setup stencil
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            // per pixel
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

            // per sample // checked Holger
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_accum_direct_msaa[0]->E[sub_phase]);
                RCache.set_CullMode(CULL_NONE);
                RCache.set_Stencil(TRUE, D3DCMP_LESS, dwLightMarkerID | 0x80, 0xff, 0x00);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            else
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_accum_direct_msaa[i]->E[sub_phase]);
                    RCache.set_CullMode(CULL_NONE);
                    RCache.set_Stencil(TRUE, D3DCMP_LESS, dwLightMarkerID | 0x80, 0xff, 0x00);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
        }

        //	Igor: draw volumetric here
        // accum_direct_volumetric	(sub_phase, Offset);
    }
}

void CRenderTarget::accum_direct_lum()
{
    PIX_EVENT(accum_direct_lum);
    //	TODO: DX10: Remove half pixel offset
    // Select target
    phase_accumulator();

    // *** assume accumulator setted up ***
    light* fuckingsun = (light*)RImplementation.Lights.sun._get();

    // Common calc for quad-rendering
    u32 Offset;
    // u32		C					= color_rgba	(255,255,255,255);
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    Fvector2 p0, p1;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);
    float d_Z = EPS_S; //, d_W = 1.f;

    // Common constants (light-related)
    Fvector L_dir, L_clr;
    float L_spec;
    L_clr.set(fuckingsun->color.r, fuckingsun->color.g, fuckingsun->color.b);
    L_spec = u_diffuse2s(L_clr);
    Device.mView.transform_dir(L_dir, fuckingsun->direction);
    L_dir.normalize();

    // recalculate d_Z, to perform depth-clipping
    Fvector center_pt;
    center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, ps_r2_sun_near);
    Device.mFullTransform.transform(center_pt);
    d_Z = center_pt.z;

    // nv-stencil recompression
    /*
    if (RImplementation.o.nvstencil  && (SE_SUN_NEAR==sub_phase))	u_stencil_optimize();	//. driver bug?
    */

    // Perform lighting
    RCache.set_CullMode(CULL_NONE);
    RCache.set_ColorWriteEnable();

    // Make jitter texture
    Fvector2 j0, j1;
    float scale_X = float(Device.dwWidth) / float(TEX_jitter);
    //		float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
    float offset = (.5f / float(TEX_jitter));
    j0.set(offset, offset);
    j1.set(scale_X, scale_X).add(offset);

    struct v_aa
    {
        Fvector4 p;
        Fvector2 uv0;
        Fvector2 uvJ;
        Fvector2 uv1;
        Fvector2 uv2;
        Fvector2 uv3;
        Fvector4 uv4;
        Fvector4 uv5;
    };
    float smooth = 0.6f;
    float ddw = smooth / _w;
    float ddh = smooth / _h;

    // Fill vertex buffer
    VERIFY(sizeof(v_aa) == g_aa_AA->vb_stride);
    v_aa* pv = (v_aa*)RCache.Vertex.Lock(4, g_aa_AA->vb_stride, Offset);
    pv->p.set(EPS, float(_h + EPS), EPS, 1.f);
    pv->uv0.set(p0.x, p1.y);
    pv->uvJ.set(j0.x, j1.y);
    pv->uv1.set(p0.x - ddw, p1.y - ddh);
    pv->uv2.set(p0.x + ddw, p1.y + ddh);
    pv->uv3.set(p0.x + ddw, p1.y - ddh);
    pv->uv4.set(p0.x - ddw, p1.y + ddh, 0, 0);
    pv->uv5.set(0, 0, 0, 0);
    pv++;
    pv->p.set(EPS, EPS, EPS, 1.f);
    pv->uv0.set(p0.x, p0.y);
    pv->uvJ.set(j0.x, j0.y);
    pv->uv1.set(p0.x - ddw, p0.y - ddh);
    pv->uv2.set(p0.x + ddw, p0.y + ddh);
    pv->uv3.set(p0.x + ddw, p0.y - ddh);
    pv->uv4.set(p0.x - ddw, p0.y + ddh, 0, 0);
    pv->uv5.set(0, 0, 0, 0);
    pv++;
    pv->p.set(float(_w + EPS), float(_h + EPS), EPS, 1.f);
    pv->uv0.set(p1.x, p1.y);
    pv->uvJ.set(j1.x, j1.y);
    pv->uv1.set(p1.x - ddw, p1.y - ddh);
    pv->uv2.set(p1.x + ddw, p1.y + ddh);
    pv->uv3.set(p1.x + ddw, p1.y - ddh);
    pv->uv4.set(p1.x - ddw, p1.y + ddh, 0, 0);
    pv->uv5.set(0, 0, 0, 0);
    pv++;
    pv->p.set(float(_w + EPS), EPS, EPS, 1.f);
    pv->uv0.set(p1.x, p0.y);
    pv->uvJ.set(j1.x, j0.y);
    pv->uv1.set(p1.x - ddw, p0.y - ddh);
    pv->uv2.set(p1.x + ddw, p0.y + ddh);
    pv->uv3.set(p1.x + ddw, p0.y - ddh);
    pv->uv4.set(p1.x - ddw, p0.y + ddh, 0, 0);
    pv->uv5.set(0, 0, 0, 0);
    pv++;
    RCache.Vertex.Unlock(4, g_aa_AA->vb_stride);
    RCache.set_Geometry(g_aa_AA);

    // setup
    RCache.set_Element(s_accum_direct->E[SE_SUN_LUMINANCE]);
    RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0.f);
    RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);

    if (!RImplementation.o.dx10_msaa)
    {
        // setup stencil
        RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }
    else
    {
        // per pixel
        RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

        // per sample
        if (RImplementation.o.dx10_msaa_opt)
        {
            RCache.set_Element(s_accum_direct_msaa[0]->E[SE_SUN_LUMINANCE]);
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
            RCache.set_CullMode(CULL_NONE);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
            {
                RCache.set_Element(s_accum_direct_msaa[i]->E[SE_SUN_LUMINANCE]);
                StateManager.SetSampleMask(u32(1) << i);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                RCache.set_CullMode(CULL_NONE);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            StateManager.SetSampleMask(0xffffffff);
        }
        RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
    }
}

void CRenderTarget::accum_direct_volumetric(u32 sub_phase, const u32 Offset, const Fmatrix& mShadow)
{
    PIX_EVENT(accum_direct_volumetric);

    if (!need_to_render_sunshafts())
        return;

    //	Test. draw only for near part
    //	if (sub_phase!=SE_SUN_N/EAR) return;
    //	if (sub_phase!=SE_SUN_FAR) return;

    if ((sub_phase != SE_SUN_NEAR) && (sub_phase != SE_SUN_FAR))
        return;

    phase_vol_accumulator();

    RCache.set_ColorWriteEnable();

    ref_selement Element = s_accum_direct_volumetric->E[0];

    const bool useMinMaxSMThisFrame = use_minmax_sm_this_frame();
    // if ( (sub_phase==SE_SUN_NEAR) && use_minmax_sm_this_frame())
    if (useMinMaxSMThisFrame)
        Element = s_accum_direct_volumetric_minmax->E[0];

    //	Assume everything was recalculated before this call by accum_direct

    // Perform lighting
    {
        // *** assume accumulator setted up ***
        light* fuckingsun = (light*)RImplementation.Lights.sun._get();

        // Common constants (light-related)
        Fvector L_clr;
        L_clr.set(fuckingsun->color.r, fuckingsun->color.g, fuckingsun->color.b);

        //	Use g_combine_2UV that was set up by accum_direct
        //	RCache.set_Geometry			(g_combine_2UV);

        // setup
        // RCache.set_Element			(s_accum_direct_volumetric->E[sub_phase]);
        RCache.set_Element(Element);
        if (useMinMaxSMThisFrame || !RImplementation.o.oldshadowcascades)
        {
            RCache.set_CullMode(CULL_CCW);
        }
        //		RCache.set_c				("Ldynamic_dir",		L_dir.x,L_dir.y,L_dir.z,0 );
        RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, 0.f);
        RCache.set_c("m_shadow", mShadow);
        Fmatrix m_Texgen;
        m_Texgen.identity();
        RCache.xforms.set_W(m_Texgen);
        RCache.xforms.set_V(Device.mView);
        RCache.xforms.set_P(Device.mProject);
        u_compute_texgen_screen(m_Texgen);

        RCache.set_c("m_texgen", m_Texgen);
        //		RCache.set_c				("m_sunmask",			m_clouds_shadow);

        // nv-DBT
        float zMin, zMax;
        if (SE_SUN_NEAR == sub_phase)
        {
            zMin = 0;
            zMax = ps_r2_sun_near;
        }
        else
        {
            extern float OLES_SUN_LIMIT_27_01_07;
            if (RImplementation.o.oldshadowcascades)
                zMin = ps_r2_sun_near;
            else
                zMin = 0; /////*****************************************************************************************
            zMax = OLES_SUN_LIMIT_27_01_07;
        }

        RCache.set_c("volume_range", zMin, zMax, 0.f, 0.f);

        Fvector center_pt;
        center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMin);
        Device.mFullTransform.transform(center_pt);
        zMin = center_pt.z;

        center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMax);
        Device.mFullTransform.transform(center_pt);
        zMax = center_pt.z;

        //	TODO: DX10: Check if DX10 has analog for NV DBT
        //		if (u_DBT_enable(zMin,zMax))	{
        // z-test always
        //			RCache.set_ZFunc(D3DCMP_ALWAYS);
        //			HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        //		}
        //		else
        {
            //	TODO: DX10: Implement via different passes
            if (SE_SUN_NEAR == sub_phase)
                RCache.set_ZFunc(D3DCMP_GREATER);
            else
                RCache.set_ZFunc(D3DCMP_ALWAYS);
        }

        // Fetch4 : enable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
        //		}

        // setup stencil: we have to draw to both lit and unlit pixels
        // RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
        // if( ! RImplementation.o.dx10_msaa )
        {
            if (RImplementation.o.oldshadowcascades)
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            else
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
        }
        /*
     else
     {
          // per pixel
          RCache.Render			(D3DPT_TRIANGLELIST,Offset,0,4,0,2);

          // per sample
        if( RImplementation.o.dx10_msaa_opt )
        {
              RCache.set_Element	(s_accum_direct_volumetric_msaa[0]->E[0]);
           RCache.set_Stencil	(TRUE,D3DCMP_ALWAYS,0xff,0xff,0xff);
               if (SE_SUN_NEAR==sub_phase)
                  RCache.set_ZFunc(D3DCMP_GREATER);
               else
                  RCache.set_ZFunc(D3DCMP_LESSEQUAL);
           RCache.Render			(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
        }
        else
        {
             for( u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i )
             {
                   RCache.set_Element	      (s_accum_direct_volumetric_msaa[i]->E[0]);
              StateManager.SetSampleMask ( u32(1) << i );
              RCache.set_Stencil	      (TRUE,D3DCMP_ALWAYS,0xff,0xff,0xff);
                   if (SE_SUN_NEAR==sub_phase)
                      RCache.set_ZFunc(D3DCMP_GREATER);
                   else
                       RCache.set_ZFunc(D3DCMP_LESSEQUAL);
              RCache.Render				   (D3DPT_TRIANGLELIST,Offset,0,4,0,2);
             }
             StateManager.SetSampleMask( 0xffffffff );
        }
          RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
     }
     */

        // Fetch4 : disable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET1  MAKEFOURCC('G','E','T','1')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
        //		}

        //	TODO: DX10: Check if DX10 has analog for NV DBT
        // disable depth bounds
        //		u_DBT_disable	();
    }
}
