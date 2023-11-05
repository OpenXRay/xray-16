#include "stdafx.h"

void CRenderTarget::draw_rain(CBackend& cmd_list, light& RainSetup)
{
    float fRainFactor = g_pGamePersistent->Environment().CurrentEnv.rain_density;

    // Common calc for quad-rendering
    u32 Offset;
    u32 C = color_rgba(255, 255, 255, 255);
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

    // recalculate d_Z, to perform depth-clipping
	float fRainFar = ps_ssfx_gloss_method == 0 ? ps_r3_dyn_wet_surf_far : 250.f;

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
        // float			fRange				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
        float fRange = 1;
        // float			fBias				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
        // float			fBias				= 0.00001;
        float fBias = -0.0001f;
        float smapsize = float(RImplementation.o.rain_smapsize);
        float fTexelOffs = (.5f / smapsize);
        //		float			view_dimX			= float(RainSetup.X.D.maxX-RainSetup.X.D.minX-2)/smapsize;
        //		float			view_dimY			= float(RainSetup.X.D.maxX-RainSetup.X.D.minX-2)/smapsize;
        //		float			view_sx				= float(RainSetup.X.D.minX+1)/smapsize;
        //		float			view_sy				= float(RainSetup.X.D.minY+1)/smapsize;
        float view_dimX = float(RainSetup.X.D[0].maxX - RainSetup.X.D[0].minX) / smapsize;
        float view_dimY = float(RainSetup.X.D[0].maxX - RainSetup.X.D[0].minX) / smapsize;
        float view_sx = float(RainSetup.X.D[0].minX) / smapsize;
        float view_sy = float(RainSetup.X.D[0].minY) / smapsize;
#if defined(USE_DX11)
        Fmatrix m_TexelAdjust =
        {
            view_dimX / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, -view_dimY / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, fRange, 0.0f,
            view_dimX / 2.f + view_sx + fTexelOffs, view_dimY / 2.f + view_sy + fTexelOffs, fBias, 1.0f
        };
#elif defined(USE_OGL)
        Fmatrix m_TexelAdjust =
        {
            view_dimX / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, view_dimY / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f * fRange, 0.0f,
            view_dimX / 2.f + view_sx + fTexelOffs, view_dimY / 2.f + view_sy + fTexelOffs, 0.5f + fBias, 1.0f
        };
#else
#   error No graphics API selected or enabled!
#endif

        // compute xforms
        FPU::m64r();

        // shadow xform
        Fmatrix m_shadow;
        {
            Fmatrix xf_project;
            xf_project.mul(m_TexelAdjust, RainSetup.X.D[0].combine);
            m_shadow.mul(xf_project, Device.mInvView);

            FPU::m24r();
        }

        /*
        // texture adjustment matrix
        //float			fRange				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
        float			fRange				=  1;
        //float			fBias				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
        //	TODO: DX11: Remove this when fix inverse culling for far region
        float			fBias				= 0;
#if defined(USE_DX11)
        Fmatrix			m_TexelAdjust		=
        {
            0.5f,				0.0f,				0.0f,			0.0f,
            0.0f,				-0.5f,				0.0f,			0.0f,
            0.0f,				0.0f,				fRange,			0.0f,
            0.5f,				0.5f,				fBias,			1.0f
        };
#elif defined(USE_OGL)
        Fmatrix			m_TexelAdjust		=
        {
            0.5f,				0.0f,				0.0f,			0.0f,
            0.0f,				0.5f,				0.0f,			0.0f,
            0.0f,				0.0f,				0.5f * fRange,			0.0f,
            0.5f,				0.5f,				0.5f + fBias,			1.0f
        };
#else
#   error No graphics API selected or enabled!
#endif

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
            //static float w_shift = 0.0f;
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
            m_clouds_shadow.mul(m_xform, Device.mInvView);
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
        FVF::TL2uv* pv = (FVF::TL2uv*)RImplementation.Vertex.Lock(3, g_combine_2UV->vb_stride, Offset);
#if defined(USE_DX11)
        pv->set(-1, -1, d_Z, d_W, C, 0, 1, 0, scale_X);
        pv++;
        pv->set(-1, 3, d_Z, d_W, C, 0, -1, 0, -scale_X);
        pv++;
        pv->set(3, -1, d_Z, d_W, C, 2, 1, 2*scale_X, scale_X);
        pv++;
#elif defined(USE_OGL)
        pv->set(-1, -1, d_Z, d_W, C, 0, 0, 0, 2*scale_X);
        pv++;
        pv->set(-1, 3, d_Z, d_W, C, 0, 2, 0, 0);
        pv++;
        pv->set(3, -1, d_Z, d_W, C, 2, 0, 2*scale_X, 2*scale_X);
        pv++;
#else
#   error No graphics API selected or enabled!
#endif
        RImplementation.Vertex.Unlock(3, g_combine_2UV->vb_stride);
        RCache.set_Geometry(g_combine_2UV);

        // setup
        // RCache.set_Element			(s_accum_direct->E[sub_phase]);
        // u_setrt	(rt_Normal,NULL,NULL,get_base_zb());
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

        //	TODO: DX11: Check if DX11 has analog for NV DBT
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
        u_setrt(cmd_list, rt_Accumulator, nullptr, nullptr, rt_MSAADepth);

        // u_setrt	(rt_Normal,NULL,NULL,get_base_zb());
        cmd_list.set_Element(s_rain->E[1]);
        cmd_list.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0.f);
        cmd_list.set_c("WorldX", W_dirX.x, W_dirX.y, W_dirX.z, 0.f);
        cmd_list.set_c("WorldZ", W_dirZ.x, W_dirZ.y, W_dirZ.z, 0.f);
        cmd_list.set_c("m_shadow", m_shadow);
        cmd_list.set_c("m_sunmask", m_clouds_shadow);
        cmd_list.set_c("RainDensity", fRainFactor, 0.f, 0.f, 0.f);
        cmd_list.set_c("RainFallof", ps_r3_dyn_wet_surf_near, ps_r3_dyn_wet_surf_far, 0.f, 0.f);
        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x01, 0);
            cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);
        }
        else
        {
            // per pixel execution
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0);
            cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);

            // per sample
            if (RImplementation.o.msaa_opt)
            {
                cmd_list.set_Element(s_rain_msaa[0]->E[0]);
                cmd_list.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0.f);
                cmd_list.set_c("WorldX", W_dirX.x, W_dirX.y, W_dirX.z, 0.f);
                cmd_list.set_c("WorldZ", W_dirZ.x, W_dirZ.y, W_dirZ.z, 0.f);
                cmd_list.set_c("m_shadow", m_shadow);
                cmd_list.set_c("m_sunmask", m_clouds_shadow);
                cmd_list.set_c("RainDensity", fRainFactor, 0.f, 0.f, 0.f);
                cmd_list.set_c("RainFallof", ps_r3_dyn_wet_surf_near, ps_r3_dyn_wet_surf_far, 0.f, 0.f);
                cmd_list.set_CullMode(CULL_NONE);
                cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);
            }
            else
            {
#if defined(USE_DX11)
                for (u32 i = 0; i < RImplementation.o.msaa_samples; ++i)
                {
                    cmd_list.set_Element(s_rain_msaa[i]->E[0]);
                    cmd_list.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0.f);
                    cmd_list.set_c("WorldX", W_dirX.x, W_dirX.y, W_dirX.z, 0.f);
                    cmd_list.set_c("WorldZ", W_dirZ.x, W_dirZ.y, W_dirZ.z, 0.f);
                    cmd_list.set_c("m_shadow", m_shadow);
                    cmd_list.set_c("m_sunmask", m_clouds_shadow);
                    cmd_list.set_c("RainDensity", fRainFactor, 0.f, 0.f, 0.f);
                    cmd_list.set_c("RainFallof", ps_r3_dyn_wet_surf_near, ps_r3_dyn_wet_surf_far, 0.f, 0.f);
                    cmd_list.StateManager.SetSampleMask(u32(1) << i);
                    cmd_list.set_CullMode(CULL_NONE);
                    cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                    cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);
                }
                cmd_list.StateManager.SetSampleMask(0xffffffff);
#elif defined(USE_OGL)
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#else
#   error No graphics API selected or enabled!
#endif
            }
        }

        //	Apply normal
        cmd_list.set_Element(s_rain->E[2]);
        cmd_list.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0.f);
        cmd_list.set_c("m_shadow", m_shadow);
        cmd_list.set_c("m_sunmask", m_clouds_shadow);

        if (!RImplementation.o.gbuffer_opt)
        {
            //	Do this in blender!
            // StateManager.SetColorWriteEnable( D3D_COLOR_WRITE_ENABLE_RED | D3D_COLOR_WRITE_ENABLE_GREEN | D3D_COLOR_WRITE_ENABLE_BLUE );
            u_setrt(cmd_list, rt_Normal, nullptr, nullptr, rt_MSAADepth);
        }
        else
        {
            // StateManager.SetColorWriteEnable( D3D_COLOR_WRITE_ENABLE_RED | D3D_COLOR_WRITE_ENABLE_GREEN );
            u_setrt(cmd_list, rt_Position, nullptr, nullptr, rt_MSAADepth);
        }

        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x01, 0);
            cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);
        }
        else
        {
            // per pixel execution
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0);
            cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);

            // per sample
            if (RImplementation.o.msaa_opt)
            {
                cmd_list.set_Element(s_rain_msaa[0]->E[1]);
                cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                cmd_list.set_CullMode(CULL_NONE);
                cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);
            }
            else
            {
#if defined(USE_DX11)
                for (u32 i = 0; i < RImplementation.o.msaa_samples; ++i)
                {
                    cmd_list.set_Element(s_rain_msaa[i]->E[1]);
                    cmd_list.StateManager.SetSampleMask(u32(1) << i);
                    cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                    cmd_list.set_CullMode(CULL_NONE);
                    cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);
                }
                cmd_list.StateManager.SetSampleMask(0xffffffff);
#elif defined(USE_OGL)
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#else
#   error No graphics API selected or enabled!
#endif
            }
        }

        //	Apply gloss
        cmd_list.set_Element(s_rain->E[3]);
        cmd_list.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0.f);
        cmd_list.set_c("m_shadow", m_shadow);
        cmd_list.set_c("m_sunmask", m_clouds_shadow);

        //	It is restored automatically by a set_Element call
        // StateManager.SetColorWriteEnable( D3D_COLOR_WRITE_ENABLE_ALL );
        u_setrt(cmd_list, rt_Color, nullptr, nullptr, rt_MSAADepth);

        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x01, 0);
            cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);
        }
        else
        {
            // per pixel execution
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0);
            cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);

            // per sample
            if (RImplementation.o.msaa_opt)
            {
                cmd_list.set_Element(s_rain_msaa[0]->E[2]);
                cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);
            }
            else
            {
#if defined(USE_DX11)
                for (u32 i = 0; i < RImplementation.o.msaa_samples; ++i)
                {
                    cmd_list.set_Element(s_rain_msaa[i]->E[2]);
                    cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);
                    cmd_list.StateManager.SetSampleMask(u32(1) << i);
                    cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 3, 0, 1);
                }
                cmd_list.StateManager.SetSampleMask(0xffffffff);
#elif defined(USE_OGL)
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#else
#   error No graphics API selected or enabled!
#endif
            }
        }

        //	TODO: DX11: Check if DX11 has analog for NV DBT
        // disable depth bounds
        //		u_DBT_disable	();
    }
}
