//////////////////////////////////////////////////
//  All comments by Nivenhbro are preceded by !
/////////////////////////////////////////////////


#ifndef SHARED_COMMON_H
#define SHARED_COMMON_H

//	Used by VS
cbuffer	dynamic_transforms : register(b0)
{
	float4x4		m_WVP;
	float4x4		m_WV;
	float4x4	    m_W;
	//	Used by VS only
	float4		L_material;	// 0,0,0,mid
	float4          hemi_cube_pos_faces;
	float4          hemi_cube_neg_faces;
};

cbuffer	shader_params : register(b1)
{
	float	m_AlphaRef;
	float4 		dt_params;	//	Detail params (moved from dynamic_transforms - material frequency)
};

cbuffer	static_globals : register(b2)
{
	float4x4		m_V;
	float4x4 	m_P;
	float4x4 	m_VP;

	float4		timers;

	float4		fog_plane;
	float4		fog_params;		// x=near*(1/(far-near)), ?,?, w = -1/(far-near)
	float4		fog_color;

	float4		L_ambient;		// L_ambient.w = skynbox-lerp-factor
	float3		L_sun_color;
	float		pbr_diffuse_mode;
	float3		L_sun_dir_w;
	float		hud_fov;
	float4		L_hemi_color;

	float3 		eye_position;
	float		_pad_eye;

	float4 		pos_decompression_params;
	float4 		pos_decompression_params2;

	float4		parallax;
	float4		screen_res;

	float4x4	m_InvVP;

	float4x4	shadow_matrices[4];
	float4		cascade_splits;

	float4		cluster_params;
	float4		cluster_scales;

	float4		camera_direction;

	float4		dev_param_1;
	float4		dev_param_2;
	float4		dev_param_3;
	float4		dev_param_4;
};

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
