#include "stdafx.h"

void CRenderTarget::draw_rain(light& RainSetup)
{
    float fRainFactor = g_pGamePersistent->Environment().CurrentEnv->rain_density;

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
    Fvector L_dir;
    Device.mView.transform_dir(L_dir, RainSetup.direction);
    L_dir.normalize();

    Fvector W_dirX;
    Device.mView.transform_dir(W_dirX, Fvector().set(1.0f, 0.0f, 0.0f));
    W_dirX.normalize();

    Fvector W_dirZ;
    Device.mView.transform_dir(W_dirZ, Fvector().set(0.0f, 0.0f, 1.0f));
    W_dirZ.normalize();

    // Perform masking (only once - on the first/near phase)
    // RCache.set_CullMode			(CULL_NONE	);
    // if (SE_SUN_NEAR==sub_phase)	//.
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
        //		float	intensity			= 0.3f*fuckingsun->color.r + 0.48f*fuckingsun->color.g +
        // 0.22f*fuckingsun->color.b;
        //		Fvector	dir					= L_dir;
        //		dir.normalize().mul	(- _sqrt(intensity+EPS));
        //		RCache.set_Element			(s_accum_mask->E[SE_MASK_DIRECT]);		// masker
        //		RCache.set_c				("Ldynamic_dir",		dir.x,dir.y,dir.z,0		);

        // if (stencil>=1 && aref_pass)	stencil = light_id
        //	Done in blender!
        // RCache.set_ColorWriteEnable	(FALSE		);
        //		RCache.set_Stencil
        //(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0x01,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
        //		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
    }

    // recalculate d_Z, to perform depth-clipping
    const float fRainFar = ps_r3_dyn_wet_surf_far;

    Fvector center_pt;
    center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, fRainFar);
    Device.mFullTransform.transform(center_pt);
    d_Z = center_pt.z;

    // nv-stencil recompression
    // if (RImplementation.o.nvstencil  && (SE_SUN_NEAR==sub_phase))	u_stencil_optimize();	//. driver bug?

    // Perform lighting
    {
        //		phase_accumulator					()	;
        //		RCache.set_CullMode					(CULL_NONE);
        //		RCache.set_ColorWriteEnable			()	;

        // texture adjustment matrix
        // float			fRange				=
        // (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
        float fRange = 1;
        // float			fBias				=
        // (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
        // float			fBias				= 0.00001;
        float fBias = -0.0001;
        float smapsize = float(RImplementation.o.smapsize);
        float fTexelOffs = (.5f / smapsize);
        //		float			view_dimX			= float(RainSetup.X.D.maxX-RainSetup.X.D.minX-2)/smapsize;
        //		float			view_dimY			= float(RainSetup.X.D.maxX-RainSetup.X.D.minX-2)/smapsize;
        //		float			view_sx				= float(RainSetup.X.D.minX+1)/smapsize;
        //		float			view_sy				= float(RainSetup.X.D.minY+1)/smapsize;
        float view_dimX = float(RainSetup.X.D.maxX - RainSetup.X.D.minX) / smapsize;
        float view_dimY = float(RainSetup.X.D.maxX - RainSetup.X.D.minX) / smapsize;
        float view_sx = float(RainSetup.X.D.minX) / smapsize;
        float view_sy = float(RainSetup.X.D.minY) / smapsize;
        Fmatrix m_TexelAdjust = {view_dimX / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, -view_dimY / 2.f, 0.0f, 0.0f, 0.0f, 0.0f,
            fRange, 0.0f, view_dimX / 2.f + view_sx + fTexelOffs, view_dimY / 2.f + view_sy + fTexelOffs, fBias, 1.0f};

        // compute xforms
        FPU::m64r();
        Fmatrix xf_invview;
        xf_invview.invert(Device.mView);

        // shadow xform
        Fmatrix m_shadow;
        {
            Fmatrix xf_project;
            xf_project.mul(m_TexelAdjust, RainSetup.X.D.combine);
            m_shadow.mul(xf_project, xf_invview);

            FPU::m24r();
        }

        /*
        // texture adjustment matrix
        //float			fRange				=
        (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
        float			fRange				=  1;
        //float			fBias				=
        (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
        //	TODO: DX10: Remove this when fix inverse culling for far region
        float			fBias				= 0;
        Fmatrix			m_TexelAdjust		=
        {
            0.5f,				0.0f,				0.0f,			0.0f,
            0.0f,				-0.5f,				0.0f,			0.0f,
            0.0f,				0.0f,				fRange,			0.0f,
            0.5f,				0.5f,				fBias,			1.0f
        };

        // compute xforms
        FPU::m64r			();
        Fmatrix				xf_invview;		xf_invview.invert	(Device.mView)	;

        // shadow xform
        Fmatrix				m_shadow;
        {
            Fmatrix			xf_project;		xf_project.mul		(m_TexelAdjust,RainSetup.X.D.combine);
            m_shadow.mul	(xf_project,	xf_invview);

            FPU::m24r		();
        }
        */

        // clouds xform
        Fmatrix m_clouds_shadow;
        {
            static float w_shift = 0;
            Fmatrix m_xform;
            // Fvector			direction	= RainSetup.direction	;
            Fvector normal;
            normal.setHP(1, 0);
            // w_shift		+=	0.003f*Device.fTimeDelta;
            // Fvector			position;	position.set(0,0,0);
            // m_xform.build_camera_dir	(position,direction,normal)	;
            m_xform.identity();
            Fvector localnormal;
            m_xform.transform_dir(localnormal, normal);
            localnormal.normalize();
            m_clouds_shadow.mul(m_xform, xf_invview);
            // m_xform.scale				(0.002f,0.002f,1.f)			;
            // 			m_xform.scale				(1.f,1.f,1.f)				;
            // 			m_clouds_shadow.mulA_44		(m_xform)					;
            // 			m_xform.translate			(localnormal.mul(w_shift))	;
            // 			m_clouds_shadow.mulA_44		(m_xform)					;
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
        // RCache.set_Element			(s_accum_direct->E[sub_phase]);
        // u_setrt	(rt_Normal,NULL,NULL,HW.pBaseZB);
        // RCache.set_Element			(s_rain->E[0]);
        // RCache.set_c				("Ldynamic_dir",		L_dir.x,L_dir.y,L_dir.z,0		);
        //		RCache.set_c				("Ldynamic_color",		L_clr.x,L_clr.y,L_clr.z,L_spec	);
        // RCache.set_c				("m_shadow",			m_shadow						);
        // RCache.set_c				("m_sunmask",			m_clouds_shadow					);

        /*
        // nv-DBT
        float zMin,zMax;
        if (SE_SUN_NEAR==sub_phase)	{
            zMin = 0;
            zMax = ps_r2_sun_near;
        } else {
            extern float	OLES_SUN_LIMIT_27_01_07;
            zMin = ps_r2_sun_near;
            zMax = OLES_SUN_LIMIT_27_01_07;
        }
        center_pt.mad(Device.vCameraPosition,Device.vCameraDirection,zMin);	Device.mFullTransform.transform	(center_pt);
        zMin = center_pt.z	;

        center_pt.mad(Device.vCameraPosition,Device.vCameraDirection,zMax);	Device.mFullTransform.transform	(center_pt);
        zMax = center_pt.z	;
        */

        //	TODO: DX10: Check if DX10 has analog for NV DBT
        //		if (u_DBT_enable(zMin,zMax))	{
        // z-test always
        //			HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        //			HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        //		}

        // Fetch4 : enable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
        //		}

        // setup stencil
        //		RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
        //		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);

        // Fetch4 : disable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET1  MAKEFOURCC('G','E','T','1')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
        //		}

        //	Use for intermediate results
        //	Patch normal
        if (!RImplementation.o.dx10_msaa)
            u_setrt(rt_Accumulator, NULL, NULL, HW.pBaseZB);
        else
            u_setrt(rt_Accumulator, NULL, NULL, rt_MSAADepth->pZRT);

        // u_setrt	(rt_Normal,NULL,NULL,HW.pBaseZB);
        RCache.set_Element(s_rain->E[1]);
        RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
        RCache.set_c("WorldX", W_dirX.x, W_dirX.y, W_dirX.z, 0);
        RCache.set_c("WorldZ", W_dirZ.x, W_dirZ.y, W_dirZ.z, 0);
        RCache.set_c("m_shadow", m_shadow);
        RCache.set_c("m_sunmask", m_clouds_shadow);
        RCache.set_c("RainDensity", fRainFactor, 0, 0, 0);
        RCache.set_c("RainFallof", ps_r3_dyn_wet_surf_near, ps_r3_dyn_wet_surf_far, 0, 0);

        if (!RImplementation.o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x01, 0);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            // per pixel execution
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

            // per sample
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_rain_msaa[0]->E[0]);
                RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
                RCache.set_c("WorldX", W_dirX.x, W_dirX.y, W_dirX.z, 0);
                RCache.set_c("WorldZ", W_dirZ.x, W_dirZ.y, W_dirZ.z, 0);
                RCache.set_c("m_shadow", m_shadow);
                RCache.set_c("m_sunmask", m_clouds_shadow);
                RCache.set_c("RainDensity", fRainFactor, 0, 0, 0);
                RCache.set_c("RainFallof", ps_r3_dyn_wet_surf_near, ps_r3_dyn_wet_surf_far, 0, 0);
                RCache.set_CullMode(CULL_NONE);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            else
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_rain_msaa[i]->E[0]);
                    RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
                    RCache.set_c("WorldX", W_dirX.x, W_dirX.y, W_dirX.z, 0);
                    RCache.set_c("WorldZ", W_dirZ.x, W_dirZ.y, W_dirZ.z, 0);
                    RCache.set_c("m_shadow", m_shadow);
                    RCache.set_c("m_sunmask", m_clouds_shadow);
                    RCache.set_c("RainDensity", fRainFactor, 0, 0, 0);
                    RCache.set_c("RainFallof", ps_r3_dyn_wet_surf_near, ps_r3_dyn_wet_surf_far, 0, 0);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.set_CullMode(CULL_NONE);
                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
        }

        //	Apply normal
        RCache.set_Element(s_rain->E[2]);
        RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
        RCache.set_c("m_shadow", m_shadow);
        RCache.set_c("m_sunmask", m_clouds_shadow);

        if (!RImplementation.o.dx10_gbuffer_opt)
        {
            //	Do this in blender!
            // StateManager.SetColorWriteEnable( D3D10_COLOR_WRITE_ENABLE_RED | D3D10_COLOR_WRITE_ENABLE_GREEN |
            // D3D10_COLOR_WRITE_ENABLE_BLUE );
            if (!RImplementation.o.dx10_msaa)
                u_setrt(rt_Normal, NULL, NULL, HW.pBaseZB);
            else
                u_setrt(rt_Normal, NULL, NULL, rt_MSAADepth->pZRT);
        }
        else
        {
            // StateManager.SetColorWriteEnable( D3D10_COLOR_WRITE_ENABLE_RED | D3D10_COLOR_WRITE_ENABLE_GREEN );
            if (!RImplementation.o.dx10_msaa)
                u_setrt(rt_Position, NULL, NULL, HW.pBaseZB);
            else
                u_setrt(rt_Position, NULL, NULL, rt_MSAADepth->pZRT);
        }

        if (!RImplementation.o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x01, 0);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            // per pixel execution
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

            // per sample
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_rain_msaa[0]->E[1]);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                RCache.set_CullMode(CULL_NONE);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            else
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_rain_msaa[i]->E[1]);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                    RCache.set_CullMode(CULL_NONE);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
        }

        //	Apply gloss
        RCache.set_Element(s_rain->E[3]);
        RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
        RCache.set_c("m_shadow", m_shadow);
        RCache.set_c("m_sunmask", m_clouds_shadow);

        //	It is restored automatically by a set_Element call
        // StateManager.SetColorWriteEnable( D3D10_COLOR_WRITE_ENABLE_ALL );
        if (!RImplementation.o.dx10_msaa)
            u_setrt(rt_Color, NULL, NULL, HW.pBaseZB);
        else
            u_setrt(rt_Color, NULL, NULL, rt_MSAADepth->pZRT);

        if (!RImplementation.o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x01, 0);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
        }
        else
        {
            // per pixel execution
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0);
            RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

            // per sample
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_rain_msaa[0]->E[2]);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
            }
            else
            {
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_rain_msaa[i]->E[2]);
                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                    StateManager.SetSampleMask(u32(1) << i);
                    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
                }
                StateManager.SetSampleMask(0xffffffff);
            }
        }

        //	TODO: DX10: Check if DX10 has analog for NV DBT
        // disable depth bounds
        //		u_DBT_disable	();
    }
}
