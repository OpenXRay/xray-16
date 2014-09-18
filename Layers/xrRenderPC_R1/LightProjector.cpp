// LightProjector.cpp: implementation of the CLightProjector class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LightProjector.h"
#include "../../include/xrRender/RenderVisual.h"
#include "../../xrEngine/xr_object.h"
#include "../xrRender/lighttrack.h"

// tir2.xrdemo		-> 45.2
// tir2.xrdemo		-> 61.8

const	float		P_distance		= 50;					// switch distance between LOW-q light and HIGH-q light
const	float		P_cam_dist		= 200;
const	float		P_cam_range		= 7.f;
const	D3DFORMAT	P_rtf			= D3DFMT_A8R8G8B8;
const	float		P_blur_kernel	= .5f;
const	int			time_min		= 30*1000	;
const	int			time_max		= 90*1000	;
const	float		P_ideal_size	= 1.f		;

float	clipD		(float R)		{ return P_distance*(R/P_ideal_size); }


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLightProjector::CLightProjector()
{
	current				= 0;
	RT					= 0;

	// 
	RT.create			("$user$projector",P_rt_size,P_rt_size,P_rtf);

	// ref-str for faster const-search
	c_xform				= "m_plmap_xform";
	c_clamp				= "m_plmap_clamp";
	c_factor			= "m_plmap_factor";

	cache.resize		(P_o_count);
	Device.seqAppActivate.Add		(this);
}

CLightProjector::~CLightProjector()
{
	Device.seqAppActivate.Remove	(this);
	RT.destroy			();
}

void CLightProjector::set_object	(IRenderable* O)
{
	if ((0==O) || (receivers.size()>=P_o_count))	current		= 0;
	else
	{
		if (!O->renderable_ShadowReceive() || RImplementation.val_bInvisible || ((CROS_impl*)O->renderable_ROS())->shadow_recv_frame==Device.dwFrame)	
		{
			current		= 0;
			return;
		}

		const vis_data &vis = O->renderable.visual->getVisData();
		Fvector		C;	O->renderable.xform.transform_tiny		(C,vis.sphere.P);
		float		R	= vis.sphere.R;
		float		D	= C.distance_to	(Device.vCameraPosition)+R;

		if (D < clipD(R))	current	= O;
		else				current = 0;
		
		if (current)				{
			ISpatial*	spatial		= dynamic_cast<ISpatial*>	(O);
			if	(0==spatial) current= 0;
			else					{
				spatial->spatial_updatesector	();
				if (0==spatial->spatial.sector)	{
					CObject*		obj = dynamic_cast<CObject*>(O);
					if (obj)		Msg	("! Invalid object '%s' position. Outside of sector structure.",obj->cName().c_str());
					current			= 0;
				}
			}
		}
		if (current)				{
			CROS_impl*	LT			= (CROS_impl*)current->renderable_ROS	();
			LT->shadow_recv_frame	= Device.dwFrame;
			receivers.push_back		(current);
		}
	}
}

// 
void CLightProjector::setup		(int id)
{
	if (id>=int(cache.size()) || id<0)	{
		// Log		("! CLightProjector::setup - ID out of range");
		return;
	}
	recv&			R			= cache[id];
	float			Rd			= R.O->renderable.visual->getVisData().sphere.R;
	float			dist		= R.C.distance_to	(Device.vCameraPosition)+Rd;
	float			factor		= _sqr(dist/clipD(Rd))*(1-ps_r1_lmodel_lerp) + ps_r1_lmodel_lerp;
	RCache.set_c	(c_xform,	R.UVgen);
	Fvector&	m	= R.UVclamp_min;
	RCache.set_ca	(c_clamp,	0,m.x,m.y,m.z,factor);
	Fvector&	M	= R.UVclamp_max;
	RCache.set_ca	(c_clamp,	1,M.x,M.y,M.z,0);
}

void CLightProjector::invalidate()
{
	for (u32 c_it=0; c_it<cache.size(); c_it++)
		cache[c_it].dwTimeValid	= 0;
}

void CLightProjector::OnAppActivate()
{
	invalidate					();
}

//
#include "../xrRender/SkeletonCustom.h"
void CLightProjector::calculate	()
{
	if (receivers.empty())		return;

	// perform validate / markup
	for (u32 r_it=0; r_it<receivers.size(); r_it++)
	{
		// validate
		BOOL				bValid	= TRUE;
		IRenderable*		O		= receivers[r_it];
		CROS_impl*			LT		= (CROS_impl*)O->renderable_ROS();
		int					slot	= LT->shadow_recv_slot;
		if (slot<0 || slot>=P_o_count)								bValid = FALSE;	// invalid slot
		else if (cache[slot].O!=O)									bValid = FALSE;	// not the same object
		else {
			// seems to be valid
			Fbox	bb;		bb.xform		(O->renderable.visual->getVisData().box,O->renderable.xform);
			if (cache[slot].BB.contains(bb))	{
				// inside, but maybe timelimit exceeded?
				if (Device.dwTimeGlobal > cache[slot].dwTimeValid)	bValid = FALSE;	// timeout
			} else													bValid = FALSE;	// out of bounds
		}

		// 
		if (bValid)			{
			// Ok, use cached version
			cache[slot].dwFrame	= Device.dwFrame;
		} else {
			taskid.push_back	(r_it);
		}
	}
	if (taskid.empty())			return;

	// Begin
	Device.Statistic->RenderDUMP_Pcalc.Begin	();
	RCache.set_RT				(RT->pRT);
	RCache.set_ZB				(RImplementation.Target->pTempZB);
	CHK_DX(HW.pDevice->Clear	(0,0, D3DCLEAR_ZBUFFER | (HW.Caps.bStencil?D3DCLEAR_STENCIL:0), 0,1,0 ));
	RCache.set_xform_world		(Fidentity);

	// reallocate/reassociate structures + perform all the work
	for (u32 c_it=0; c_it<cache.size(); c_it++)
	{
		if (taskid.empty())							break;
		if (Device.dwFrame==cache[c_it].dwFrame)	continue;

		// found not used slot
		int				tid		= taskid.back();	taskid.pop_back();
		recv&			R		= cache		[c_it];
		IRenderable*	O		= receivers	[tid];
		const vis_data& vis = O->renderable.visual->getVisData();
		CROS_impl*	LT		= (CROS_impl*)O->renderable_ROS();
		VERIFY2			(_valid(O->renderable.xform),"Invalid object transformation");
		VERIFY2			(_valid(vis.sphere.P),"Invalid object's visual sphere");

		Fvector			C;		O->renderable.xform.transform_tiny		(C,vis.sphere.P);
		R.O						= O;
		R.C						= C;
		R.C.y					+= vis.sphere.R*0.1f;		//. YURA: 0.1 can be more
		R.BB.xform				(vis.box,O->renderable.xform).scale(0.1f);
		R.dwTimeValid			= Device.dwTimeGlobal + ::Random.randI(time_min,time_max);
		LT->shadow_recv_slot	= c_it; 

		// Msg					("[%f,%f,%f]-%f",C.C.x,C.C.y,C.C.z,C.O->renderable.visual->vis.sphere.R);
		// calculate projection-matrix
		Fmatrix		mProject;
		float		p_R			=	R.O->renderable.visual->getVisData().sphere.R * 1.1f;
		//VERIFY2		(p_R>EPS_L,"Object has no physical size");
		VERIFY3		(p_R>EPS_L,"Object has no physical size", R.O->renderable.visual->getDebugName().c_str());
		float		p_hat		=	p_R/P_cam_dist;
		float		p_asp		=	1.f;
		float		p_near		=	P_cam_dist-EPS_L;									
		float		p_far		=	P_cam_dist+p_R+P_cam_range;	
		mProject.build_projection_HAT	(p_hat,p_asp,p_near,p_far);
		RCache.set_xform_project		(mProject);
		
		// calculate view-matrix
		Fmatrix		mView;
		Fvector		v_C, v_Cs, v_N;
		v_C.set					(R.C);
		v_Cs					= v_C;
		v_C.y					+=	P_cam_dist;
		v_N.set					(0,0,1);
		VERIFY					(_valid(v_C) && _valid(v_Cs) && _valid(v_N));

		// validate
		Fvector		v;
		v.sub		(v_Cs,v_C);;
#ifdef DEBUG
		if ((v.x*v.x+v.y*v.y+v.z*v.z)<=flt_zero)	{
			CObject* OO = dynamic_cast<CObject*>(R.O);
			Msg("Object[%s] Visual[%s] has invalid position. ",*OO->cName(),*OO->cNameVisual());
			Fvector cc;
			OO->Center(cc);
			Log("center=",cc);

			Log("visual_center=",OO->Visual()->getVisData().sphere.P);
			
			Log("full_matrix=",OO->XFORM());

			Log	("v_N",v_N);
			Log	("v_C",v_C);
			Log	("v_Cs",v_Cs);

			Log("all bones transform:--------");
			CKinematics* K = dynamic_cast<CKinematics*>(OO->Visual());
			
			for(u16 ii=0; ii<K->LL_BoneCount();++ii){
				Fmatrix tr;

				tr = K->LL_GetTransform(ii);
				Log("bone ",K->LL_BoneName_dbg(ii));
				Log("bone_matrix",tr);
			}
			Log("end-------");
		}
#endif
		// handle invalid object-bug
		if ((v.x*v.x+v.y*v.y+v.z*v.z)<=flt_zero)	{
			// invalidate record, so that object will be unshadowed, but doesn't crash
			R.dwTimeValid			= Device.dwTimeGlobal;
			LT->shadow_recv_frame	= Device.dwFrame-1;
			LT->shadow_recv_slot	= -1; 
			continue				;
		}

		mView.build_camera		(v_C,v_Cs,v_N);
		RCache.set_xform_view	(mView);

		// Select slot, set viewport
		int		s_x				=	c_it%P_o_line;
		int		s_y				=	c_it/P_o_line;
		D3DVIEWPORT9 VP			=	{s_x*P_o_size,s_y*P_o_size,P_o_size,P_o_size,0,1 };
		CHK_DX					(HW.pDevice->SetViewport(&VP));

		// Clear color to ambience
		Fvector&	cap			=	LT->get_approximate();
		CHK_DX					(HW.pDevice->Clear(0,0, D3DCLEAR_TARGET, color_rgba_f(cap.x,cap.y,cap.z, (cap.x+cap.y+cap.z)/4.f), 1, 0 ));

		// calculate uv-gen matrix and clamper
		Fmatrix					mCombine;		mCombine.mul	(mProject,mView);
		Fmatrix					mTemp;
		float					fSlotSize		= float(P_o_size)/float(P_rt_size);
		float					fSlotX			= float(s_x*P_o_size)/float(P_rt_size);
		float					fSlotY			= float(s_y*P_o_size)/float(P_rt_size);
		float					fTexelOffs		= (.5f / P_rt_size);
		Fmatrix					m_TexelAdjust	= 
		{
			0.5f/*x-scale*/,	0.0f,							0.0f,				0.0f,
			0.0f,				-0.5f/*y-scale*/,				0.0f,				0.0f,
			0.0f,				0.0f,							1.0f/*z-range*/,	0.0f,
			0.5f/*x-bias*/,		0.5f + fTexelOffs/*y-bias*/,	0.0f/*z-bias*/,		1.0f
		};
		R.UVgen.mul				(m_TexelAdjust,mCombine);
		mTemp.scale				(fSlotSize,fSlotSize,1);
		R.UVgen.mulA_44			(mTemp);
		mTemp.translate			(fSlotX+fTexelOffs,fSlotY+fTexelOffs,0);
		R.UVgen.mulA_44			(mTemp);

		// Build bbox and render
		Fvector					min,max;
		Fbox					BB;
		min.set					(R.C.x-p_R,	R.C.y-(p_R+P_cam_range),	R.C.z-p_R);
		max.set					(R.C.x+p_R,	R.C.y+0,					R.C.z+p_R);
		BB.set					(min,max);
		R.UVclamp_min.set		(min).add	(.05f);	// shrink a little
		R.UVclamp_max.set		(max).sub	(.05f);	// shrink a little
		ISpatial*	spatial		= dynamic_cast<ISpatial*>	(O);
		if (spatial)			{
			spatial->spatial_updatesector			();
			if (spatial->spatial.sector)			RImplementation.r_dsgraph_render_R1_box	(spatial->spatial.sector,BB,SE_R1_LMODELS);
		}
		//if (spatial)		RImplementation.r_dsgraph_render_subspace	(spatial->spatial.sector,mCombine,v_C,FALSE);
	}

	// Blur
	/*
	{
		// Fill vertex buffer
		u32							Offset;
		FVF::TL4uv* pv				= (FVF::TL4uv*) RCache.Vertex.Lock	(4,geom_Blur.stride(),Offset);
		RImplementation.ApplyBlur4	(pv,P_rt_size,P_rt_size,P_blur_kernel);
		RCache.Vertex.Unlock		(4,geom_Blur.stride());

		// Actual rendering (pass0, temp2real)
		RCache.set_RT				(RT->pRT);
		RCache.set_ZB				(NULL);
		RCache.set_Shader			(sh_BlurTR	);
		RCache.set_Geometry			(geom_Blur	);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}
	*/

	// Finita la comedia
	Device.Statistic->RenderDUMP_Pcalc.End	();
	
	RCache.set_xform_project	(Device.mProject);
	RCache.set_xform_view		(Device.mView);
}

#ifdef DEBUG
void CLightProjector::render	()
{
	/*
	#define CLS(a) color_rgba(a,a,a,a)
	RCache.set_xform_world		(Fidentity);
	Device.Resources->OnFrameEnd	();
	for (u32 it=0; it<boxes.size(); it++)
	{
		Fvector C,D; boxes[it].get_CD	(C,D);
		RCache.dbg_DrawAABB	(C,D.x,D.y,D.z,0xffffffff);
	}
	boxes.clear();

	// Debug
	{
		// UV
		Fvector2				p0,p1;
		p0.set					(.5f/P_rt_size, .5f/P_rt_size);
		p1.set					((P_rt_size+.5f)/P_rt_size, (P_rt_size+.5f)/P_rt_size);
		
		// Fill vertex buffer
		u32 C			=	0xffffffff, Offset;
		u32 _w		=	P_rt_size/2, _h = P_rt_size/2;
		FVF::TL* pv		=	(FVF::TL*) geom_Screen->Lock(4,Offset);
		pv->set(0,			float(_h),	.0001f,.9999f, C, p0.x, p1.y);	pv++;
		pv->set(0,			0,			.0001f,.9999f, C, p0.x, p0.y);	pv++;
		pv->set(float(_w),	float(_h),	.0001f,.9999f, C, p1.x, p1.y);	pv++;
		pv->set(float(_w),	0,			.0001f,.9999f, C, p1.x, p0.y);	pv++;
		geom_Screen->Unlock			(4);
		
		// Actual rendering
		RCache.set_Shader(sh_Screen);
		RCache.Draw	(geom_Screen,4,2,Offset,Device.Streams_QuadIB);
	}
	*/
}
#endif
