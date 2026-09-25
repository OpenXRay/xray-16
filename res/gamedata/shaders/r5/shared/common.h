
#ifndef SHARED_COMMON_H
#define SHARED_COMMON_H

cbuffer	dynamic_transforms : register(b0)
{
	float4x4		m_WVP;
	float4x4		m_WV;
	float4x4	    m_W;
	float4		L_material;
	float4          hemi_cube_pos_faces;
	float4          hemi_cube_neg_faces;
};

cbuffer	shader_params : register(b1)
{
	float	m_AlphaRef;
	float4 		dt_params;
};

cbuffer	static_globals : register(b2)
{
	float4x4		m_V;
	float4x4 	m_P;
	float4x4 	m_VP;

	float4		timers;

	float4		fog_plane;
	float4		fog_params;
	float4		fog_color;

	float4		L_ambient;
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

	float4		foliage_sss;
	float4		foliage_params;
	float4		foliage_params2;
	float4		sky_ibl;
};

float 	calc_cyclic 	(float x)				
{
	float 	phase 	= 1/(2*3.141592653589f);
	float 	sqrt2	= 1.4142136f;
	float 	sqrt2m2	= 2.8284271f;
	float 	f 	= sqrt2m2*frac(x)-sqrt2;
	return 	f*f - 1.f;
}

float2 	calc_xz_wave 	(float2 dir2D, float frac)		
{
	float2  ctrl_A	= float2(0.f,		0.f	);
	float2 	ctrl_B	= float2(dir2D.x,	dir2D.y	);
	return  lerp	(ctrl_A, ctrl_B, frac);
}


#endif
