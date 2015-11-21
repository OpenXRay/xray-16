#include "stdafx.h"
#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/environment.h"

void CRenderTarget::accum_direct		(u32 sub_phase)
{
	// Choose normal code-path or filtered
	phase_accumulator					();
	if (RImplementation.o.sunfilter)	{
		accum_direct_f	(sub_phase);
		return			;
	}

	//	TODO: DX10: Remove half pixe offset
	// *** assume accumulator setted up ***
	light*			fuckingsun			= (light*)RImplementation.Lights.sun_adapted._get()	;

	// Common calc for quad-rendering
	u32		Offset;
	u32		C					= color_rgba	(255,255,255,255);
	float	_w					= float			(Device.dwWidth);
	float	_h					= float			(Device.dwHeight);
	Fvector2					p0,p1;
	p0.set						(.5f/_w, .5f/_h);
	p1.set						((_w+.5f)/_w, (_h+.5f)/_h );
	float	d_Z	= EPS_S, d_W = 1.f;

	// Common constants (light-related)
	Fvector		L_dir,L_clr;	float L_spec;
	L_clr.set					(fuckingsun->color.r,fuckingsun->color.g,fuckingsun->color.b);
	L_spec						= u_diffuse2s	(L_clr);
	Device.mView.transform_dir	(L_dir,fuckingsun->direction);
	L_dir.normalize				();

	// Perform masking (only once - on the first/near phase)
	RCache.set_CullMode			(CULL_NONE	);
	if (SE_SUN_NEAR==sub_phase)	//.
   //if( 0 )
	{
		// Fill vertex buffer
		FVF::TL* pv					= (FVF::TL*)	RCache.Vertex.Lock	(4,g_combine->vb_stride,Offset);
		pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p0.y);	pv++;
		pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p1.y);	pv++;
		pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p0.y);	pv++;
		pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p1.y);	pv++;
		RCache.Vertex.Unlock		(4,g_combine->vb_stride);
		RCache.set_Geometry			(g_combine);

		// setup
		float	intensity			= 0.3f*fuckingsun->color.r + 0.48f*fuckingsun->color.g + 0.22f*fuckingsun->color.b;
		Fvector	dir					= L_dir;
				dir.normalize().mul	(- _sqrt(intensity+EPS));
		RCache.set_Element			(s_accum_mask->E[SE_MASK_DIRECT]);		// masker
		RCache.set_c				("Ldynamic_dir",		dir.x,dir.y,dir.z,0		);

		// if (stencil>=1 && aref_pass)	stencil = light_id
		RCache.set_ColorWriteEnable	(FALSE		);
		RCache.set_Stencil		(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0x01,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}

	// recalculate d_Z, to perform depth-clipping
	Fvector	center_pt;			center_pt.mad	(Device.vCameraPosition,Device.vCameraDirection,ps_r2_sun_near);
	Device.mFullTransform.transform(center_pt)	;
	d_Z							= center_pt.z	;

	// nv-stencil recompression
	if (RImplementation.o.nvstencil  && (SE_SUN_NEAR==sub_phase))	u_stencil_optimize();	//. driver bug?

	// Perform lighting
	{
		phase_accumulator					()	;
		RCache.set_CullMode					(CULL_NONE);
		RCache.set_ColorWriteEnable			()	;

		// texture adjustment matrix
		//float			fTexelOffs			= (.5f / float(RImplementation.o.smapsize));
		//float			fRange				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
		//float			fBias				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
		//Fmatrix			m_TexelAdjust		= 
		//{
		//	0.5f,				0.0f,				0.0f,			0.0f,
		//	0.0f,				0.5f,				0.0f,			0.0f,
		//	0.0f,				0.0f,				fRange,			0.0f,
		//	0.5f + fTexelOffs,	0.5f + fTexelOffs,	fBias,			1.0f
		//};
		float			fRange				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
		//float			fBias				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
		//	TODO: DX10: Remove this when fix inverse culling for far region
		float			fBias				= (SE_SUN_NEAR==sub_phase)?(-ps_r2_sun_depth_near_bias):ps_r2_sun_depth_far_bias;
		Fmatrix			m_TexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				0.5f * fRange,	0.0f,
			0.5f,				0.5f,				0.5f + fBias,	1.0f
		};

		// compute xforms
		FPU::m64r			();
		Fmatrix				xf_invview;		xf_invview.invert	(Device.mView)	;

		// shadow xform
		Fmatrix				m_shadow;
		{
			Fmatrix			xf_project;		xf_project.mul		(m_TexelAdjust,fuckingsun->X.D.combine);
			m_shadow.mul	(xf_project,	xf_invview);

			// tsm-bias
			if ( (SE_SUN_FAR == sub_phase) && (RImplementation.o.HW_smap) )
			{
				Fvector		bias;	bias.mul		(L_dir,ps_r2_sun_tsm_bias);
				Fmatrix		bias_t;	bias_t.translate(bias);
				m_shadow.mulB_44	(bias_t);
			}
			FPU::m24r		();
		}

		// clouds xform
		Fmatrix				m_clouds_shadow;
		{
			static	float	w_shift		= 0;
			Fmatrix			m_xform;
			Fvector			direction	= fuckingsun->direction	;
			float	w_dir				= g_pGamePersistent->Environment().CurrentEnv->wind_direction	;
			//float	w_speed				= g_pGamePersistent->Environment().CurrentEnv->wind_velocity	;
			Fvector			normal	;	normal.setHP(w_dir,0);
							w_shift		+=	0.003f*Device.fTimeDelta;
			Fvector			position;	position.set(0,0,0);
			m_xform.build_camera_dir	(position,direction,normal)	;
			Fvector			localnormal;m_xform.transform_dir(localnormal,normal); localnormal.normalize();
			m_clouds_shadow.mul			(m_xform,xf_invview)		;
			m_xform.scale				(0.002f,0.002f,1.f)			;
			m_clouds_shadow.mulA_44		(m_xform)					;
			m_xform.translate			(localnormal.mul(w_shift))	;
			m_clouds_shadow.mulA_44		(m_xform)					;
		}

		// Make jitter texture
		Fvector2					j0,j1;
		float	scale_X				= float(Device.dwWidth)	/ float(TEX_jitter);
		//float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
		float	offset				= (.5f / float(TEX_jitter));
		j0.set						(offset,offset);
		j1.set						(scale_X,scale_X).add(offset);

		// Fill vertex buffer
		FVF::TL2uv* pv				= (FVF::TL2uv*) RCache.Vertex.Lock	(4,g_combine_2UV->vb_stride,Offset);
		//pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y, j0.x, j1.y);	pv++;
		//pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y, j0.x, j0.y);	pv++;
		//pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y, j1.x, j1.y);	pv++;
		//pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y, j1.x, j0.y);	pv++;
		pv->set						(-1,	-1,	d_Z,	d_W, C, 0, 0, 0,		0);	pv++;
		pv->set						(-1,	1,	d_Z,	d_W, C, 0, 1, 0,		scale_X);	pv++;
		pv->set						(1,		-1,	d_Z,	d_W, C, 1, 0, scale_X,	0);	pv++;
		pv->set						(1,		1,	d_Z,	d_W, C, 1, 1, scale_X,	scale_X);	pv++;
		RCache.Vertex.Unlock		(4,g_combine_2UV->vb_stride);
		RCache.set_Geometry			(g_combine_2UV);

		// setup
		RCache.set_Element			(s_accum_direct->E[sub_phase]);
		RCache.set_c				("Ldynamic_dir",		L_dir.x,L_dir.y,L_dir.z,0		);
		RCache.set_c				("Ldynamic_color",		L_clr.x,L_clr.y,L_clr.z,L_spec	);
		RCache.set_c				("m_shadow",			m_shadow						);
		RCache.set_c				("m_sunmask",			m_clouds_shadow					);
		
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
		RCache.set_Stencil	(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
		RCache.Render			(D3DPT_TRIANGLELIST,Offset,0,4,0,2);

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
		//if (ps_r2_ls_flags.test(R2FLAG_SUN_SHAFTS))
		if ( RImplementation.o.advancedpp&&(ps_r_sun_shafts>0))
			accum_direct_volumetric	(sub_phase, Offset, m_shadow);
	}
}

void CRenderTarget::accum_direct_blend	()
{
	// blend-copy
	if (!RImplementation.o.fp16_blend)	{
		VERIFY(0);
		u_setrt						(rt_Accumulator,NULL,NULL,HW.pBaseZB);

		//	TODO: DX10: remove half pixel offset
		// Common calc for quad-rendering
		u32		Offset;
		u32		C					= color_rgba	(255,255,255,255);
		float	_w					= float			(Device.dwWidth);
		float	_h					= float			(Device.dwHeight);
		Fvector2					p0,p1;
		p0.set						(.5f/_w, .5f/_h);
		p1.set						((_w+.5f)/_w, (_h+.5f)/_h );
		float	d_Z	= EPS_S, d_W = 1.f;

		// Fill vertex buffer
		FVF::TL2uv* pv				= (FVF::TL2uv*) RCache.Vertex.Lock	(4,g_combine_2UV->vb_stride,Offset);
		//pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y, p0.x, p1.y);	pv++;
		//pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y, p0.x, p0.y);	pv++;
		//pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y, p1.x, p1.y);	pv++;
		//pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y, p1.x, p0.y);	pv++;
		pv->set						(-1,	-1,	d_Z,	d_W, C, 0, 0, 0,	0);	pv++;
		pv->set						(-1,	1,	d_Z,	d_W, C, 0, 1, 0,	1);	pv++;
		pv->set						(1,		-1,	d_Z,	d_W, C, 1, 0, 1,	0);	pv++;
		pv->set						(1,		1,	d_Z,	d_W, C, 1, 1, 1,	1);	pv++;
		RCache.Vertex.Unlock		(4,g_combine_2UV->vb_stride);
		RCache.set_Geometry			(g_combine_2UV);
		RCache.set_Element			(s_accum_mask->E[SE_MASK_ACCUM_2D]	);
		RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2	);
	}
	//dwLightMarkerID				+= 2;
	increment_light_marker();
}

void CRenderTarget::accum_direct_f		(u32 sub_phase)
{
	// Select target
	if (SE_SUN_LUMINANCE==sub_phase)	{
		accum_direct_lum	();
		return				;
	}
	phase_accumulator					();
	u_setrt								(rt_Generic_0,NULL,NULL,HW.pBaseZB);

	// *** assume accumulator setted up ***
	light*			fuckingsun			= (light*)RImplementation.Lights.sun_adapted._get()	;

	// Common calc for quad-rendering
	u32		Offset;
	u32		C					= color_rgba	(255,255,255,255);
	float	_w					= float			(Device.dwWidth);
	float	_h					= float			(Device.dwHeight);
	Fvector2					p0,p1;
	p0.set						(.5f/_w, .5f/_h);
	p1.set						((_w+.5f)/_w, (_h+.5f)/_h );
	float	d_Z	= EPS_S, d_W = 1.f;

	// Common constants (light-related)
	Fvector		L_dir,L_clr;	float L_spec;
	L_clr.set					(fuckingsun->color.r,fuckingsun->color.g,fuckingsun->color.b);
	L_spec						= u_diffuse2s	(L_clr);
	Device.mView.transform_dir	(L_dir,fuckingsun->direction);
	L_dir.normalize				();

	// Perform masking (only once - on the first/near phase)
	RCache.set_CullMode			(CULL_NONE	);
	if (SE_SUN_NEAR==sub_phase)	//.
	{
		// For sun-filter - clear to zero
		CHK_DX	(HW.pDevice->Clear	( 0L, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0L));

		// Fill vertex buffer
		FVF::TL* pv					= (FVF::TL*)	RCache.Vertex.Lock	(4,g_combine->vb_stride,Offset);
		pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p0.y);	pv++;
		pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p1.y);	pv++;
		pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p0.y);	pv++;
		pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p1.y);	pv++;
		RCache.Vertex.Unlock		(4,g_combine->vb_stride);
		RCache.set_Geometry			(g_combine);

		// setup
		float	intensity			= 0.3f*fuckingsun->color.r + 0.48f*fuckingsun->color.g + 0.22f*fuckingsun->color.b;
		Fvector	dir					= L_dir;
		dir.normalize().mul	(- _sqrt(intensity+EPS));
		RCache.set_Element			(s_accum_mask->E[SE_MASK_DIRECT]);		// masker
		RCache.set_c				("Ldynamic_dir",		dir.x,dir.y,dir.z,0		);

		// if (stencil>=1 && aref_pass)	stencil = light_id
		RCache.set_ColorWriteEnable	(FALSE		);
		RCache.set_Stencil	(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0x01,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		RCache.Render		(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}

	// recalculate d_Z, to perform depth-clipping
	Fvector	center_pt;			center_pt.mad	(Device.vCameraPosition,Device.vCameraDirection,ps_r2_sun_near);
	Device.mFullTransform.transform(center_pt)	;
	d_Z							= center_pt.z	;

	// nv-stencil recompression
	if (RImplementation.o.nvstencil  && (SE_SUN_NEAR==sub_phase))	u_stencil_optimize();	//. driver bug?

	// Perform lighting
	{
		u_setrt								(rt_Generic_0,NULL,NULL,HW.pBaseZB);  // enshure RT setup
		RCache.set_CullMode					(CULL_NONE	);
		RCache.set_ColorWriteEnable			();

		// texture adjustment matrix
		float			fTexelOffs			= (.5f / float(RImplementation.o.smapsize));
		float			fRange				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
		//float			fBias				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
		//	TODO: DX10: Remove this when fix inverse culling for far region
		float			fBias				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:-ps_r2_sun_depth_far_bias;
		Fmatrix			m_TexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				0.5f * fRange,	0.0f,
			0.5f + fTexelOffs,	0.5f + fTexelOffs,	0.5f + fBias,	1.0f
		};

		// compute xforms
		Fmatrix				m_shadow;
		{
			FPU::m64r		();
			Fmatrix			xf_invview;		xf_invview.invert	(Device.mView)	;
			Fmatrix			xf_project;		xf_project.mul		(m_TexelAdjust,fuckingsun->X.D.combine);
			m_shadow.mul	(xf_project,	xf_invview);

			// tsm-bias
			if (SE_SUN_FAR == sub_phase)
			{
				Fvector		bias;	bias.mul		(L_dir,ps_r2_sun_tsm_bias);
				Fmatrix		bias_t;	bias_t.translate(bias);
				m_shadow.mulB_44	(bias_t);
			}
			FPU::m24r		();
		}

		// Make jitter texture
		Fvector2					j0,j1;
		float	scale_X				= float(Device.dwWidth)	/ float(TEX_jitter);
		//float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
		float	offset				= (.5f / float(TEX_jitter));
		j0.set						(offset,offset);
		j1.set						(scale_X,scale_X).add(offset);

		// Fill vertex buffer
		FVF::TL2uv* pv				= (FVF::TL2uv*) RCache.Vertex.Lock	(4,g_combine_2UV->vb_stride,Offset);
		//pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y, j0.x, j1.y);	pv++;
		//pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y, j0.x, j0.y);	pv++;
		//pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y, j1.x, j1.y);	pv++;
		//pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y, j1.x, j0.y);	pv++;
		pv->set						(-1,	-1,	d_Z,	d_W, C, 0, 0, 0,		0);	pv++;
		pv->set						(-1,	1,	d_Z,	d_W, C, 0, 1, 0,		scale_X);	pv++;
		pv->set						(1,		-1,	d_Z,	d_W, C, 1, 0, scale_X,	0);	pv++;
		pv->set						(1,		1,	d_Z,	d_W, C, 1, 1, scale_X,	scale_X);	pv++;
		RCache.Vertex.Unlock		(4,g_combine_2UV->vb_stride);
		RCache.set_Geometry			(g_combine_2UV);

		// setup
		RCache.set_Element			(s_accum_direct->E[sub_phase]);
		RCache.set_c				("Ldynamic_dir",		L_dir.x,L_dir.y,L_dir.z,0		);
		RCache.set_c				("Ldynamic_color",		L_clr.x,L_clr.y,L_clr.z,L_spec	);
		RCache.set_c				("m_shadow",			m_shadow						);

		// setup stencil
		RCache.set_Stencil	(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
		RCache.Render			(D3DPT_TRIANGLELIST,Offset,0,4,0,2);

		//	Igor: draw volumetric here
		//accum_direct_volumetric	(sub_phase, Offset);
	}
}

void CRenderTarget::accum_direct_lum	()
{
	//	TODO: DX10: Remove half pixel offset
	// Select target
	phase_accumulator					();

	// *** assume accumulator setted up ***
	light*			fuckingsun			= (light*)RImplementation.Lights.sun_adapted._get()	;

	// Common calc for quad-rendering
	u32		Offset;
	// u32		C					= color_rgba	(255,255,255,255);
	float	_w					= float			(Device.dwWidth);
	float	_h					= float			(Device.dwHeight);
	Fvector2					p0,p1;
	p0.set						(.5f/_w, .5f/_h);
	p1.set						((_w+.5f)/_w, (_h+.5f)/_h );
	float	d_Z	= EPS_S;		//, d_W = 1.f;

	// Common constants (light-related)
	Fvector		L_dir,L_clr;	float L_spec;
	L_clr.set					(fuckingsun->color.r,fuckingsun->color.g,fuckingsun->color.b);
	L_spec						= u_diffuse2s	(L_clr);
	Device.mView.transform_dir	(L_dir,fuckingsun->direction);
	L_dir.normalize				();

	// recalculate d_Z, to perform depth-clipping
	Fvector	center_pt;			center_pt.mad	(Device.vCameraPosition,Device.vCameraDirection,ps_r2_sun_near);
	Device.mFullTransform.transform(center_pt)	;
	d_Z							= center_pt.z	;

	// nv-stencil recompression
	/*
	if (RImplementation.o.nvstencil  && (SE_SUN_NEAR==sub_phase))	u_stencil_optimize();	//. driver bug?
	*/

	// Perform lighting
		RCache.set_CullMode					(CULL_NONE	);
		RCache.set_ColorWriteEnable			();

		// Make jitter texture
		Fvector2					j0,j1;
		float	scale_X				= float(Device.dwWidth)	/ float(TEX_jitter);
//		float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
		float	offset				= (.5f / float(TEX_jitter));
		j0.set						(offset,offset);
		j1.set						(scale_X,scale_X).add(offset);

		struct v_aa	{
			Fvector4	p;
			Fvector2	uv0;
			Fvector2	uvJ;
			Fvector2	uv1;
			Fvector2	uv2;
			Fvector2	uv3;
			Fvector4	uv4;
			Fvector4	uv5;
		};
		float	smooth				= 0.6f;
		float	ddw					= smooth/_w;
		float	ddh					= smooth/_h;

		// Fill vertex buffer
		VERIFY	(sizeof(v_aa)==g_aa_AA->vb_stride);
		v_aa* pv					= (v_aa*) RCache.Vertex.Lock	(4,g_aa_AA->vb_stride,Offset);
		pv->p.set(EPS,			float(_h+EPS),	EPS,1.f); pv->uv0.set(p0.x, p1.y);pv->uvJ.set(j0.x, j1.y);pv->uv1.set(p0.x-ddw,p1.y-ddh);pv->uv2.set(p0.x+ddw,p1.y+ddh);pv->uv3.set(p0.x+ddw,p1.y-ddh);pv->uv4.set(p0.x-ddw,p1.y+ddh,0,0);pv->uv5.set(0,0,0,0);pv++;
		pv->p.set(EPS,			EPS,			EPS,1.f); pv->uv0.set(p0.x, p0.y);pv->uvJ.set(j0.x, j0.y);pv->uv1.set(p0.x-ddw,p0.y-ddh);pv->uv2.set(p0.x+ddw,p0.y+ddh);pv->uv3.set(p0.x+ddw,p0.y-ddh);pv->uv4.set(p0.x-ddw,p0.y+ddh,0,0);pv->uv5.set(0,0,0,0);pv++;
		pv->p.set(float(_w+EPS),float(_h+EPS),	EPS,1.f); pv->uv0.set(p1.x, p1.y);pv->uvJ.set(j1.x, j1.y);pv->uv1.set(p1.x-ddw,p1.y-ddh);pv->uv2.set(p1.x+ddw,p1.y+ddh);pv->uv3.set(p1.x+ddw,p1.y-ddh);pv->uv4.set(p1.x-ddw,p1.y+ddh,0,0);pv->uv5.set(0,0,0,0);pv++;
		pv->p.set(float(_w+EPS),EPS,			EPS,1.f); pv->uv0.set(p1.x, p0.y);pv->uvJ.set(j1.x, j0.y);pv->uv1.set(p1.x-ddw,p0.y-ddh);pv->uv2.set(p1.x+ddw,p0.y+ddh);pv->uv3.set(p1.x+ddw,p0.y-ddh);pv->uv4.set(p1.x-ddw,p0.y+ddh,0,0);pv->uv5.set(0,0,0,0);pv++;
		RCache.Vertex.Unlock		(4,g_aa_AA->vb_stride);
		RCache.set_Geometry			(g_aa_AA);

		// setup
		RCache.set_Element	(s_accum_direct->E[SE_SUN_LUMINANCE]);
		RCache.set_c				("Ldynamic_dir",		L_dir.x,L_dir.y,L_dir.z,0		);
		RCache.set_c				("Ldynamic_color",		L_clr.x,L_clr.y,L_clr.z,L_spec	);

		// setup stencil
		RCache.set_Stencil	(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
		RCache.Render			(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
}

void CRenderTarget::accum_direct_volumetric	(u32 sub_phase, const u32 Offset, const Fmatrix &mShadow)
{
	if (!need_to_render_sunshafts())
		return;
	
	//	Test. draw only for near part
//	if (sub_phase!=SE_SUN_N/EAR) return;
//	if (sub_phase!=SE_SUN_FAR) return;

	if ( (sub_phase!=SE_SUN_NEAR) && (sub_phase!=SE_SUN_FAR) ) return;

	phase_vol_accumulator();

	RCache.set_ColorWriteEnable();

	// Perform lighting
	{

		// *** assume accumulator setted up ***
		light*			fuckingsun			= (light*)RImplementation.Lights.sun_adapted._get()	;

		// Common constants (light-related)
		Fvector		L_clr;
		L_clr.set					(fuckingsun->color.r,fuckingsun->color.g,fuckingsun->color.b);
		
		//	Use g_combine_2UV that was set up by accum_direct
		//	RCache.set_Geometry			(g_combine_2UV);

		// setup
		//RCache.set_Element			(s_accum_direct_volumetric->E[sub_phase]);
		RCache.set_Element			(s_accum_direct_volumetric->E[0]);
//		RCache.set_c				("Ldynamic_dir",		L_dir.x,L_dir.y,L_dir.z,0 );
		RCache.set_c				("Ldynamic_color",		L_clr.x,L_clr.y,L_clr.z,0);
		RCache.set_c				("m_shadow",			mShadow);
//		RCache.set_c				("m_sunmask",			m_clouds_shadow);

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

		RCache.set_c("volume_range", zMin, zMax, 0, 0);

		Fvector	center_pt;
		center_pt.mad(Device.vCameraPosition,Device.vCameraDirection,zMin);	
		Device.mFullTransform.transform(center_pt);
		zMin = center_pt.z	;

		center_pt.mad(Device.vCameraPosition,Device.vCameraDirection,zMax);	
		Device.mFullTransform.transform	(center_pt);
		zMax = center_pt.z	;

//	TODO: DX10: Check if DX10 has analog for NV DBT
//		if (u_DBT_enable(zMin,zMax))	{
			// z-test always
//			HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
//			HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
//		}
//		else
		{

//	TODO: DX10: Implement via different passes
			if (SE_SUN_NEAR==sub_phase)
				//HW.pDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_GREATER);
				RCache.set_ZFunc(D3DCMP_GREATER);
			else
				//HW.pDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
				RCache.set_ZFunc(D3DCMP_LESSEQUAL);
		}

		// Fetch4 : enable
//		if (RImplementation.o.HW_smap_FETCH4)	{
			//. we hacked the shader to force smap on S0
//#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4') 
//			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
//		}

		// setup stencil: we have to draw to both lit and unlit pixels
		//RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);

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
