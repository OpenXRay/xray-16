//////////////////////////////////////////////////
//  All comments by Nivenhbro are preceded by !
/////////////////////////////////////////////////


#ifndef SHARED_COMMON_H
#define SHARED_COMMON_H

//	Used by VS
cbuffer	dynamic_transforms
{
	uniform float4x4		m_WVP;		//	World View Projection composition
	uniform float3x4		m_WV;
	uniform float3x4	    m_W;
	//	Used by VS only
	uniform float4		L_material;	// 0,0,0,mid
	uniform float4          hemi_cube_pos_faces;
	uniform float4          hemi_cube_neg_faces;
	uniform	float4 		dt_params;	//	Detail params
}

cbuffer	shader_params
{
	float	m_AlphaRef;
}

cbuffer	static_globals
{
	uniform float3x4		m_V;
	uniform float4x4 	m_P;
	uniform float4x4 	m_VP;

	uniform float4		timers;

	uniform float4		fog_plane;
	uniform float4		fog_params;		// x=near*(1/(far-near)), ?,?, w = -1/(far-near)
	uniform float4		fog_color;

	uniform float4		L_ambient;		// L_ambient.w = skynbox-lerp-factor
	uniform float3		L_sun_color;
	uniform float3		L_sun_dir_w;
	uniform float4		L_hemi_color;

	uniform float3 		eye_position;

	uniform float4 		pos_decompression_params;
	uniform float4 		pos_decompression_params2;

	uniform float4		parallax;
//	uniform float4		screen_res;		// Screen resolution (x-Width,y-Height, zw - 1/resolution)
}

/*
//




uniform float4x4 	m_texgen;
//uniform float4x4 	mVPTexgen;
uniform float3		L_sun_dir_e;

//uniform float3		eye_direction;
uniform float3		eye_normal;
*/

float 	calc_cyclic 	(float x)				
{
	float 	phase 	= 1/(2*3.141592653589f);
	float 	sqrt2	= 1.4142136f;
	float 	sqrt2m2	= 2.8284271f;
	float 	f 	= sqrt2m2*frac(x)-sqrt2;	// [-sqrt2 .. +sqrt2] !No changes made, but this controls the grass wave (which is violent if I must say)
	return 	f*f - 1.f;				// [-1     .. +1]
}

float2 	calc_xz_wave 	(float2 dir2D, float frac)		
{
	// Beizer
	float2  ctrl_A	= float2(0.f,		0.f	);
	float2 	ctrl_B	= float2(dir2D.x,	dir2D.y	);
	return  lerp	(ctrl_A, ctrl_B, frac);			//!This calculates tree wave. No changes made
}


#endif
