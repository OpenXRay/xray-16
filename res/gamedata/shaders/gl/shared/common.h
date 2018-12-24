//////////////////////////////////////////////////
//  All comments by Nivenhbro are preceded by !
/////////////////////////////////////////////////


#ifndef SHARED_COMMON_H
#define SHARED_COMMON_H

#define half        float
#define half2       vec2
#define half3       vec3
#define half4       vec4
#define float2      vec2
#define float3      vec3
#define float4      vec4
#define int2        ivec2
#define int3        ivec3
#define int4        ivec4
#define uint2       uvec2
#define uint3       uvec3
#define uint4       uvec4
#define half3x3     mat3
#define half4x4     mat4
#define half3x4     mat4x3
#define float3x3    mat3
#define float4x4    mat4
#define float3x4    mat4x3

vec4	mul(int a,		vec4 b)	{ return a * b; }
vec4	mul(float a,	vec4 b)	{ return a * b; }
vec3	mul(mat3 a,		vec3 b)	{ return a * b; }
vec3	mul(vec3 a,		mat3 b)	{ return a * b; }
mat3	mul(mat3 a,		mat3 b)	{ return a * b; }
vec4	mul(mat4 a,		vec4 b)	{ return a * b; }
vec4	mul(vec4 a,		mat4 b)	{ return a * b; }
mat4	mul(mat4 a,		mat4 b)	{ return a * b; }
vec3	mul(mat4x3 a,	vec4 b)	{ return a * b; }
vec3	mul(mat4x3 a,	vec3 b)	{ return mat3(a) * b; }
void	sincos(float x, out float s, out float c) { s = sin(x); c = cos(x); }

#define lerp        mix
#define frac        fract
#define saturate(a) clamp(a, 0.0, 1.0)
#define clip(x)		if (x < 0) discard
#define tex2D		texture
#define tex2Dproj	textureProj
#define tex2Dlod(s,t)	textureLod(s,t.xy,t.w)
#define tex3D		texture
#define texCUBE 	texture
#define asuint		floatBitsToUint
#define asfloat		uintBitsToFloat
#define mask(m,a,b)	mix(b,a,m)

// Semantics assignment, maximum 16 slots
#define COLOR		0
#define COLOR0		0
#define COLOR1		1
#define COLOR2		2
#define POSITION	3
#define POSITIONT	3
#define POSITION0	3
#define POSITION1	4
#define TANGENT		4
#define NORMAL		5
#define NORMAL0		5
#define NORMAL1		6
#define BINORMAL	6
#define FOG		7
#define TEXCOORD0	8
#define TEXCOORD1	9
#define TEXCOORD2	10
#define TEXCOORD3	11
#define TEXCOORD4	12
#define TEXCOORD5	13
#define TEXCOORD6	14
#define TEXCOORD7	15

//	Used by VS
// TODO: OGL: Use constant buffers.
//cbuffer	dynamic_transforms
//{
	uniform float4x4		m_WVP;		//	World View Projection composition
	uniform float3x4		m_WV;
	uniform float3x4	    m_W;
	//	Used by VS only
	uniform float4		L_material;	// 0,0,0,mid
	uniform float4          hemi_cube_pos_faces;
	uniform float4          hemi_cube_neg_faces;
	uniform	float4 		dt_params;	//	Detail params
//}

// TODO: OGL: Implement reference alpha.
/*cbuffer	shader_params
{
	float	m_AlphaRef;
}*/

// TODO: OGL: Use constant buffers.
//cbuffer	static_globals
//{
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
//}

float 	calc_cyclic 	(float x)				
{
	float 	phase 	= 1/(2*3.141592653589);
	float 	sqrt2	= 1.4142136;
	float 	sqrt2m2	= 2.8284271;
	float 	f 	= sqrt2m2*frac(x)-sqrt2;	// [-sqrt2 .. +sqrt2] !No changes made, but this controls the grass wave (which is violent if I must say)
	return 	f*f - 1.0;				// [-1     .. +1]
}

float2 	calc_xz_wave 	(float2 dir2D, float frac)		
{
	// Beizer
	float2  ctrl_A	= float2(0.0,		0.0	);
	float2 	ctrl_B	= float2(dir2D.x,	dir2D.y	);
	return  lerp	(ctrl_A, ctrl_B, frac);			//!This calculates tree wave. No changes made
}

#endif
