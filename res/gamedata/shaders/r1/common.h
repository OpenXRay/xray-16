#ifndef	COMMON_H
#define COMMON_H

#include "shared\common.h"

uniform half4		L_dynamic_props;	// per object, xyz=sun,w=hemi
uniform half4		L_dynamic_color;	// dynamic light color (rgb1)	- spot/point
uniform half4		L_dynamic_pos;		// dynamic light pos+1/range(w) - spot/point
uniform float4x4 	L_dynamic_xform;

uniform float4x4	m_plmap_xform;
uniform float4 		m_plmap_clamp	[2];	// 0.w = factor

half  	calc_fogging 	(half4 w_pos)	{ return dot(w_pos,fog_plane); 	}
half2 	calc_detail 	(half3 w_pos)	{ 
	float  	dtl	= distance(w_pos,eye_position)*dt_params.w;
		dtl	= min(dtl*dtl, 1);
	half  	dt_mul	= 1  - dtl;	// dt*  [1 ..  0 ]
	half  	dt_add	= .5 * dtl;	// dt+	[0 .. 0.5]
	return	half2	(dt_mul,dt_add);
}
float3 	calc_reflection	(float3 pos_w, float3 norm_w)
{
    return reflect(normalize(pos_w-eye_position), norm_w);
}
float4	calc_spot 	(out float4 tc_lmap, out float2 tc_att, float4 w_pos, float3 w_norm)	{
	float4 	s_pos	= mul	(L_dynamic_xform, w_pos);
	tc_lmap		= s_pos.xyww;			// projected in ps/ttf
	tc_att 		= s_pos.z;			// z=distance * (1/range)
	float3 	L_dir_n = normalize	(w_pos - L_dynamic_pos.xyz);
	float 	L_scale	= dot(w_norm,-L_dir_n);
	return 	L_dynamic_color*L_scale*saturate(calc_fogging(w_pos));
}
float4	calc_point 	(out float2 tc_att0, out float2 tc_att1, float4 w_pos, float3 w_norm)	{
	float3 	L_dir_n = normalize	(w_pos - L_dynamic_pos.xyz);
	float 	L_scale	= dot		(w_norm,-L_dir_n);
	float3	L_tc 	= (w_pos - L_dynamic_pos.xyz) * L_dynamic_pos.w + .5f;	// tc coords
	tc_att0		= L_tc.xz;
	tc_att1		= L_tc.xy;
	return 	L_dynamic_color*L_scale*saturate(calc_fogging(w_pos));
}
float3	calc_sun		(float3 norm_w)	{ return L_sun_color*max(dot((norm_w),-L_sun_dir_w),0); 		}
float3 	calc_model_hemi 	(float3 norm_w)	{ return (norm_w.y*0.5+0.5)*L_dynamic_props.w*L_hemi_color; 		}
float3	calc_model_lq_lighting	(float3 norm_w) { return calc_model_hemi(norm_w) + L_ambient + L_dynamic_props.xyz*calc_sun(norm_w); 	}
float3 	_calc_model_hemi 	(float3 norm_w)	{ return max(0,norm_w.y)*.2*L_hemi_color; 				}
float3	_calc_model_lq_lighting	(float3 norm_w) { return calc_model_hemi(norm_w) + L_ambient + .5*calc_sun(norm_w); 	}
float4 	calc_model_lmap 	(float3 pos_w)	{
	float3  pos_wc	= clamp		(pos_w,m_plmap_clamp[0],m_plmap_clamp[1]);		// clamp to BBox
	float4 	pos_w4c	= float4	(pos_wc,1);	
	float4 	plmap 	= mul		(m_plmap_xform,pos_w4c);				// calc plmap tc
	return  plmap.xyww;
}

struct 	v_lmap
{
	float4 	P	: POSITION;			// (float,float,float,1)
	float4	N	: NORMAL;			// (nx,ny,nz,hemi occlusion)
	float4 	T	: TANGENT;
	float4 	B	: BINORMAL;
	float2 	uv0	: TEXCOORD0;		// (base)
	float2	uv1	: TEXCOORD1;		// (lmap/compressed)
};
struct	v_vert
{
	float4 	P		: POSITION;		// (float,float,float,1)
	float4	N		: NORMAL;		// (nx,ny,nz,hemi occlusion)
	float4 	T		: TANGENT;
	float4 	B		: BINORMAL;
	float4	color	: COLOR0;		// (r,g,b,dir-occlusion)
	float2 	uv0		: TEXCOORD0;	// (u0,v0)
};
struct 	v_model
{
	float4 	pos	: POSITION;	// (float,float,float,1)
	float3	norm	: NORMAL;	// (nx,ny,nz)
	float3	T	: TANGENT;	// (nx,ny,nz)
	float3	B	: BINORMAL;	// (nx,ny,nz)
	float2	tc	: TEXCOORD0;	// (u,v)
#ifdef SKIN_COLOR
	float3 	rgb_tint;
#endif
};
struct	v_detail
{
	float4 	pos	: POSITION;	// (float,float,float,1)
	int4 	misc	: TEXCOORD0;	// (u(Q),v(Q),frac,matrix-id)
};
struct 	vf_spot
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;	// base
	float4 tc1	: TEXCOORD1;	// lmap, projected
	float2 tc2	: TEXCOORD2;	// att + clipper
#ifdef DL_DETAILS
	float2 tcd	: TEXCOORD3;	// details
#endif
	float4 color: COLOR0;
};
struct 	vf_point
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;	// base
	float2 tc1	: TEXCOORD1;	// att1 + clipper
	float2 tc2	: TEXCOORD2;	// att2 + clipper
#ifdef DL_DETAILS
	float2 tcd	: TEXCOORD3;	// details
#endif
	float4 color: COLOR0;
};
//////////////////////////////////////////////////////////////////////////////////////////
uniform sampler2D 	s_base;
uniform samplerCUBE 	s_env;
uniform sampler2D 	s_lmap;
uniform sampler2D 	s_hemi;
uniform sampler2D 	s_att;
uniform sampler2D 	s_detail;

#define def_distort	half(0.05f)	// we get -0.5 .. 0.5 range, this is -512 .. 512 for 1024, so scale it

float3	v_hemi 		(float3 n)		{	return L_hemi_color/* *(.5f + .5f*n.y) */; 		}
float3	v_hemi_wrap	(float3 n, float w)	{	return L_hemi_color/* *(w + (1-w)*n.y) */; 		}
float3 	v_sun 		(float3 n)		{	return L_sun_color*max(0,dot(n,-L_sun_dir_w));		}
float3 	v_sun_wrap	(float3 n, float w)	{	return L_sun_color*(w+(1-w)*dot(n,-L_sun_dir_w));	}
half3	p_hemi		(float2 tc) 	{
	//half3	t_lmh 	= tex2D		(s_hemi, tc);
	//return  dot	(t_lmh,1.h/3.h);
	half4	t_lmh 	= tex2D		(s_hemi, tc);
	return  t_lmh.a;
}

#endif // COMMON_H
