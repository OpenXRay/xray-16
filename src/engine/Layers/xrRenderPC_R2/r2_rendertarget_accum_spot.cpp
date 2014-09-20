#include "stdafx.h"
#include "../xrRender/du_cone.h"

//extern Fvector du_cone_vertices[DU_CONE_NUMVERTEX];

void CRenderTarget::accum_spot	(light* L)
{
	phase_accumulator				();
	RImplementation.stats.l_visible	++;

	// *** assume accumulator setted up ***
	// *****************************	Mask by stencil		*************************************
	ref_shader			shader;
	if (IRender_Light::OMNIPART == L->flags.type)	{
			shader		= L->s_point;
		if (!shader)	shader		= s_accum_point;
	} else {
			shader		= L->s_spot;
		if (!shader)	shader		= s_accum_spot;
	}

	BOOL	bIntersect			= FALSE; //enable_scissor(L);
	{
		// setup xform
		L->xform_calc					();
		RCache.set_xform_world			(L->m_xform			);
		RCache.set_xform_view			(Device.mView		);
		RCache.set_xform_project		(Device.mProject	);
		bIntersect						= enable_scissor	(L);
		enable_dbt_bounds				(L);

		// *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
		// *** thus can cope without stencil clear with 127 lights
		// *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
		RCache.set_ColorWriteEnable		(FALSE);
		RCache.set_Element				(s_accum_mask->E[SE_MASK_SPOT]);		// masker

		// backfaces: if (stencil>=1 && zfail)			stencil = light_id
		RCache.set_CullMode				(CULL_CW);
		RCache.set_Stencil				(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0x01,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE);
		draw_volume						(L);

		// frontfaces: if (stencil>=light_id && zfail)	stencil = 0x1
		RCache.set_CullMode				(CULL_CCW);
		RCache.set_Stencil				(TRUE,D3DCMP_LESSEQUAL,0x01,0xff,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE);
		draw_volume						(L);
	}

	// nv-stencil recompression
	if (RImplementation.o.nvstencil)	u_stencil_optimize();

	// *****************************	Minimize overdraw	*************************************
	// Select shader (front or back-faces), *** back, if intersect near plane
	RCache.set_ColorWriteEnable		();
	RCache.set_CullMode				(CULL_CW);		// back

	// 2D texgens 
	Fmatrix			m_Texgen;			u_compute_texgen_screen	(m_Texgen	);
	Fmatrix			m_Texgen_J;			u_compute_texgen_jitter	(m_Texgen_J	);

	// Shadow xform (+texture adjustment matrix)
	Fmatrix			m_Shadow,m_Lmap;
	{
		float			smapsize			= float(RImplementation.o.smapsize);
		float			fTexelOffs			= (.5f / smapsize);
		float			view_dim			= float(L->X.S.size-2)/smapsize;
		float			view_sx				= float(L->X.S.posX+1)/smapsize;
		float			view_sy				= float(L->X.S.posY+1)/smapsize;
		float			fRange				= float(1.f)*ps_r2_ls_depth_scale;
		float			fBias				= ps_r2_ls_depth_bias;
		Fmatrix			m_TexelAdjust		= {
			view_dim/2.f,							0.0f,									0.0f,		0.0f,
			0.0f,									-view_dim/2.f,							0.0f,		0.0f,
			0.0f,									0.0f,									fRange,		0.0f,
			view_dim/2.f + view_sx + fTexelOffs,	view_dim/2.f + view_sy + fTexelOffs,	fBias,		1.0f
		};

		// compute xforms
		Fmatrix			xf_world;		xf_world.invert	(Device.mView);
		Fmatrix			xf_view			= L->X.S.view;
		Fmatrix			xf_project;		xf_project.mul	(m_TexelAdjust,L->X.S.project);
		m_Shadow.mul					(xf_view, xf_world);
		m_Shadow.mulA_44				(xf_project	);

		// lmap
						view_dim			= 1.f;
						view_sx				= 0.f;
						view_sy				= 0.f;
		Fmatrix			m_TexelAdjust2		= {
			view_dim/2.f,							0.0f,									0.0f,		0.0f,
			0.0f,									-view_dim/2.f,							0.0f,		0.0f,
			0.0f,									0.0f,									fRange,		0.0f,
			view_dim/2.f + view_sx + fTexelOffs,	view_dim/2.f + view_sy + fTexelOffs,	fBias,		1.0f
		};

		// compute xforms
		xf_project.mul		(m_TexelAdjust2,L->X.S.project);
		m_Lmap.mul			(xf_view, xf_world);
		m_Lmap.mulA_44		(xf_project	);
	}

	// Common constants
	Fvector		L_dir,L_clr,L_pos;	float L_spec;
	L_clr.set					(L->color.r,L->color.g,L->color.b);
	L_clr.mul					(L->get_LOD());
	L_spec						= u_diffuse2s	(L_clr);
	Device.mView.transform_tiny	(L_pos,L->position);
	Device.mView.transform_dir	(L_dir,L->direction);
	L_dir.normalize				();

	// Draw volume with projective texgen
	{
		// Select shader
		u32		_id					= 0;
		if (L->flags.bShadow)		{
			bool	bFullSize			= (L->X.S.size == RImplementation.o.smapsize);
			if (L->X.S.transluent)	_id	= SE_L_TRANSLUENT;
			else if		(bFullSize)	_id	= SE_L_FULLSIZE;
			else					_id	= SE_L_NORMAL;
		} else {
			_id						= SE_L_UNSHADOWED;
			m_Shadow				= m_Lmap;
		}
		RCache.set_Element			(shader->E[ _id ]	);

		// Constants
		float	att_R				= L->range*.95f;
		float	att_factor			= 1.f/(att_R*att_R);
		RCache.set_c				("Ldynamic_pos",	L_pos.x,L_pos.y,L_pos.z,att_factor);
		RCache.set_c				("Ldynamic_color",	L_clr.x,L_clr.y,L_clr.z,L_spec);
		RCache.set_c				("m_texgen",		m_Texgen	);
		RCache.set_c				("m_texgen_J",		m_Texgen_J	);
		RCache.set_c				("m_shadow",		m_Shadow	);
		RCache.set_ca				("m_lmap",		0,	m_Lmap._11, m_Lmap._21, m_Lmap._31, m_Lmap._41	);
		RCache.set_ca				("m_lmap",		1,	m_Lmap._12, m_Lmap._22, m_Lmap._32, m_Lmap._42	);

		// Fetch4 : enable
		if (RImplementation.o.HW_smap_FETCH4)	{
			//. we hacked the shader to force smap on S0
#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4') 
			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
		}

		RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
		draw_volume					(L);

		// Fetch4 : disable
		if (RImplementation.o.HW_smap_FETCH4)	{
			//. we hacked the shader to force smap on S0
#			define FOURCC_GET1  MAKEFOURCC('G','E','T','1') 
			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
		}
	}

	// blend-copy
	if (!RImplementation.o.fp16_blend)	{
		u_setrt						(rt_Accumulator,NULL,NULL,HW.pBaseZB);
		RCache.set_Element			(s_accum_mask->E[SE_MASK_ACCUM_VOL]	);
		RCache.set_c				("m_texgen",		m_Texgen);
		RCache.set_c				("m_texgen_J",		m_Texgen_J	);
		draw_volume					(L);
	}
	CHK_DX		(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE));
	//dwLightMarkerID					+=	2;	// keep lowest bit always setted up
	increment_light_marker();

	u_DBT_disable				();
}

void CRenderTarget::accum_volumetric(light* L)
{
	//if (L->flags.type != IRender_Light::SPOT) return;
	if (!L->flags.bVolumetric) return;

	phase_vol_accumulator();

	ref_shader			shader;
	shader = L->s_volumetric;
	if (!shader)
		shader = s_accum_volume;

	// *** assume accumulator setted up ***
	// *****************************	Mask by stencil		*************************************
	BOOL	bIntersect			= FALSE; //enable_scissor(L);
	{
		// setup xform
		L->xform_calc					();
		RCache.set_xform_world			(L->m_xform			);
		RCache.set_xform_view			(Device.mView		);
		RCache.set_xform_project		(Device.mProject	);
		bIntersect						= enable_scissor	(L);

		//enable_dbt_bounds				(L);
	}

	RCache.set_ColorWriteEnable		();
	RCache.set_CullMode				(CULL_NONE);		// back

	// 2D texgens 
	Fmatrix			m_Texgen;			u_compute_texgen_screen	(m_Texgen	);
	Fmatrix			m_Texgen_J;			u_compute_texgen_jitter	(m_Texgen_J	);

	// Shadow xform (+texture adjustment matrix)
	Fmatrix			m_Shadow,m_Lmap;
	Fmatrix			mFrustumSrc;
	CFrustum		ClipFrustum;
	{
		float			smapsize			= float(RImplementation.o.smapsize);
		float			fTexelOffs			= (.5f / smapsize);
		float			view_dim			= float(L->X.S.size-2)/smapsize;
		float			view_sx				= float(L->X.S.posX+1)/smapsize;
		float			view_sy				= float(L->X.S.posY+1)/smapsize;
		float			fRange				= float(1.f)*ps_r2_ls_depth_scale;
		float			fBias				= ps_r2_ls_depth_bias;
		Fmatrix			m_TexelAdjust		= {
			view_dim/2.f,							0.0f,									0.0f,		0.0f,
			0.0f,									-view_dim/2.f,							0.0f,		0.0f,
			0.0f,									0.0f,									fRange,		0.0f,
			view_dim/2.f + view_sx + fTexelOffs,	view_dim/2.f + view_sy + fTexelOffs,	fBias,		1.0f
		};

		// compute xforms
		Fmatrix			xf_world;		xf_world.invert	(Device.mView);
		Fmatrix			xf_view			= L->X.S.view;
		Fmatrix			xf_project;		xf_project.mul	(m_TexelAdjust,L->X.S.project);
		m_Shadow.mul					(xf_view, xf_world);
		m_Shadow.mulA_44				(xf_project	);

		// lmap
		view_dim			= 1.f;
		view_sx				= 0.f;
		view_sy				= 0.f;
		Fmatrix			m_TexelAdjust2		= {
			view_dim/2.f,							0.0f,									0.0f,		0.0f,
			0.0f,									-view_dim/2.f,							0.0f,		0.0f,
			0.0f,									0.0f,									fRange,		0.0f,
			view_dim/2.f + view_sx + fTexelOffs,	view_dim/2.f + view_sy + fTexelOffs,	fBias,		1.0f
		};

		// compute xforms
		xf_project.mul		(m_TexelAdjust2,L->X.S.project);
		m_Lmap.mul			(xf_view, xf_world);
		m_Lmap.mulA_44		(xf_project	);

		// Compute light frustum in world space
		mFrustumSrc.mul(L->X.S.project, xf_view);
		ClipFrustum.CreateFromMatrix( mFrustumSrc, FRUSTUM_P_ALL);
		//	Adjust frustum far plane
		//	4 - far, 5 - near
		ClipFrustum.planes[4].d -= 
			(ClipFrustum.planes[4].d + ClipFrustum.planes[5].d)*(1-L->m_volumetric_distance);
		
	}

	//	Calculate camera space AABB
	// xform BB
	/*
	Fbox	BB;
	Fvector	rr; rr.set(L->spatial.sphere.R,L->spatial.sphere.R,L->spatial.sphere.R);
	BB.setb	(L->spatial.sphere.P, rr);

	Fbox	bbp; bbp.invalidate();
	for (u32 i=0; i<8; i++)		{
		Fvector		pt;
		BB.getpoint	(i,pt);
		//Device.mFullTransform.transform	(pt);
		Device.mFullTransform.transform	(mView);
		bbp.modify	(pt);
	}
	*/

	
	//	Calculate camera space AABB
	//	Adjust AABB according to the adjusted distance for the light volume
	Fbox	aabb;
	
	//float	scaledRadius = L->spatial.sphere.R * (1+L->m_volumetric_distance)*0.5f;
	float	scaledRadius = L->spatial.sphere.R * L->m_volumetric_distance;
	Fvector	rr = Fvector().set(scaledRadius,scaledRadius,scaledRadius);
	Fvector pt = L->spatial.sphere.P;
	pt.sub(L->position);
	pt.mul(L->m_volumetric_distance);
	pt.add(L->position);
	//	Don't adjust AABB
	//float	scaledRadius = L->spatial.sphere.R;
	//Fvector	rr = Fvector().set(scaledRadius,scaledRadius,scaledRadius);
	//Fvector pt = L->spatial.sphere.P;
	Device.mView.transform(pt);
	aabb.setb( pt, rr);
/*	
	//	Calculate presise AABB assuming we are drawing for the spot light
	{
		aabb.invalidate();
		Fmatrix	transform;
		transform.mul( Device.mView, L->m_xform);		 
		for (u32 i=0; i<DU_CONE_NUMVERTEX; ++i)
		{
			Fvector		pt = du_cone_vertices[i];
			transform.transform(pt);
			aabb.modify(pt);
		}
		
	}
*/
	// Common constants
	float		fQuality = L->m_volumetric_quality;
	int			iNumSlises = (int)(VOLUMETRIC_SLICES*fQuality);
	//			min 10 surfaces
	iNumSlises = _max(10, iNumSlises);
	//	Adjust slice intensity
	fQuality	= ((float)iNumSlises)/VOLUMETRIC_SLICES;
	Fvector		L_dir,L_clr,L_pos;	float L_spec;
	L_clr.set					(L->color.r,L->color.g,L->color.b);
	L_clr.mul					(L->m_volumetric_intensity);
	L_clr.mul					(L->m_volumetric_distance);
	L_clr.mul					(1/fQuality);
	L_clr.mul					(L->get_LOD());
	L_spec						= u_diffuse2s	(L_clr);
	Device.mView.transform_tiny	(L_pos,L->position);
	Device.mView.transform_dir	(L_dir,L->direction);
	L_dir.normalize				();

	// Draw volume with projective texgen
	{
		/*
		// Select shader
		u32		_id					= 0;
		if (L->flags.bShadow)		{
			bool	bFullSize			= (L->X.S.size == RImplementation.o.smapsize);
			if (L->X.S.transluent)	_id	= SE_L_TRANSLUENT;
			else if		(bFullSize)	_id	= SE_L_FULLSIZE;
			else					_id	= SE_L_NORMAL;
		} else {
			_id						= SE_L_UNSHADOWED;
			m_Shadow				= m_Lmap;
		}
		RCache.set_Element			(shader->E[ _id ]	);
		*/

		//	Set correct depth surface
		//	It's slow. Make this when shader is created
		{
			char*		pszSMapName;
			BOOL		b_HW_smap	= RImplementation.o.HW_smap;
			BOOL		b_HW_PCF	= RImplementation.o.HW_smap_PCF;
			if (b_HW_smap)		{
				if (b_HW_PCF)	pszSMapName = r2_RT_smap_depth;
				else			pszSMapName = r2_RT_smap_depth;
			}
			else				pszSMapName = r2_RT_smap_surf;
			//s_smap
			STextureList* _T = &*s_accum_volume->E[0]->passes[0]->T;

			STextureList::iterator	_it		= _T->begin	();
			STextureList::iterator	_end	= _T->end	();
			for (; _it!=_end; _it++)
			{
				std::pair<u32,ref_texture>&		loader	=	*_it;
				u32			load_id	= loader.first;
				//	Shadowmap texture always uses 0 texture unit
				if (load_id==0)		
				{
					//	Assign correct texture
					loader.second.create(pszSMapName);
				}
			}
		}
		

		RCache.set_Element			(shader->E[0]);

		// Constants
		float	att_R				= L->m_volumetric_distance*L->range*.95f;
		float	att_factor			= 1.f/(att_R*att_R);
		RCache.set_c				("Ldynamic_pos",	L_pos.x,L_pos.y,L_pos.z,att_factor);
		RCache.set_c				("Ldynamic_color",	L_clr.x,L_clr.y,L_clr.z,L_spec);
		RCache.set_c				("m_texgen",		m_Texgen	);
		RCache.set_c				("m_texgen_J",		m_Texgen_J	);
		RCache.set_c				("m_shadow",		m_Shadow	);
		RCache.set_ca				("m_lmap",		0,	m_Lmap._11, m_Lmap._21, m_Lmap._31, m_Lmap._41	);
		RCache.set_ca				("m_lmap",		1,	m_Lmap._12, m_Lmap._22, m_Lmap._32, m_Lmap._42	);
		RCache.set_c				("vMinBounds",		aabb.x1, aabb.y1, aabb.z1, 0);
		//	Increase camera-space aabb z size to compensate decrease of slices number
		RCache.set_c				("vMaxBounds",		aabb.x2, aabb.y2, aabb.z1 + (aabb.z2-aabb.z1)/fQuality, 0);

		//	Set up user clip planes
		{
			//	Transform frustum to clip space
			Fmatrix PlaneTransform;
			PlaneTransform.transpose(Device.mInvFullTransform);
			HW.pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0x3F);

			for ( int i=0; i<6; ++i)
			{
				Fvector4	&ClipPlane = *(Fvector4*)&ClipFrustum.planes[i].n.x;
				Fvector4	TransformedPlane;
				PlaneTransform.transform(TransformedPlane, ClipPlane);
				TransformedPlane.mul(-1.0f);
				HW.pDevice->SetClipPlane( i, &TransformedPlane.x);
			}
		}
		

		/*
		float	clip[4];
		clip[0] = 1;
		clip[1] = 
		clip[2] = 
		clip[3] = 0;
		HW.pDevice->SetClipPlane( 0, clip);
		*/


		// Fetch4 : enable
		if (RImplementation.o.HW_smap_FETCH4)	{
			//. we hacked the shader to force smap on S0
#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4') 
			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
		}

		RCache.set_ColorWriteEnable(D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE);

		RCache.set_Geometry(g_accum_volumetric);
		//RCache.Render(D3DPT_TRIANGLELIST,0,0,VOLUMETRIC_SLICES*4,0,VOLUMETRIC_SLICES*2);
		RCache.Render(D3DPT_TRIANGLELIST,0,0,iNumSlises*4,0,iNumSlises*2);

		RCache.set_ColorWriteEnable();

		// Fetch4 : disable
		if (RImplementation.o.HW_smap_FETCH4)	{
			//. we hacked the shader to force smap on S0
#			define FOURCC_GET1  MAKEFOURCC('G','E','T','1') 
			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
		}

		//	Restore clip planes
		HW.pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
	}
/*
	// blend-copy
	if (!RImplementation.o.fp16_blend)	{
		u_setrt						(rt_Accumulator,NULL,NULL,HW.pBaseZB);
		RCache.set_Element			(s_accum_mask->E[SE_MASK_ACCUM_VOL]	);
		RCache.set_c				("m_texgen",		m_Texgen);
		RCache.set_c				("m_texgen_J",		m_Texgen_J	);
		draw_volume					(L);
	}
*/
	CHK_DX		(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE));
}