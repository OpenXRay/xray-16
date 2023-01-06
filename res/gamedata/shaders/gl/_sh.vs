#version 400
#pragma optimize (off)
// water_soft
#define SMAP_size	2048
#define FP16_FILTER	1
#define FP16_BLEND	1
#define USE_HWSMAP	1
#define USE_HWSMAP_PCF	1
#define USE_BRANCHING	1
#define USE_VTF	1
#define SKIN_NONE	1
#define USE_SOFT_WATER	1
#define SSR_QUALITY	3
#define SSR_HALF_DEPTH	1
#define SSR_JITTER	1
#define USE_SOFT_PARTICLES	1
#define USE_DOF	1
#define SUN_SHAFTS_QUALITY	1
#define SSAO_QUALITY	1
#define SUN_QUALITY	1
#define ALLOW_STEEPPARALLAX	1
#define GBUFFER_OPTIMIZATION	1
#define SM_4_1	1
#define	NEED_SOFT_WATER
#ifndef        COMMON_H
#define        COMMON_H

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

vec4	mul(int a,		vec4 b)	{ return float(a) * b; }
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
	float 	phase 	= 1.0/(2.0*3.141592653589);
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
 


#ifndef	common_defines_h_included
#define	common_defines_h_included

//////////////////////////////////////////////////////////////////////////////////////////
// Defines                                		//
#define def_gloss       float(2.0/255.0)
#define def_aref        float(200.0/255.0)
#define def_dbumph      float(0.333)
#define def_virtualh    float(0.05)              // 5cm
#define def_distort     float(0.05)             // we get -0.5 .. 0.5 range, this is -512 .. 512 for 1024, so scale it
#define def_hdr         float(9.0)         		// hight luminance range float(3.0)
#define def_hdr_clip	float(0.75)        		//

#define	LUMINANCE_VECTOR	float3(0.3, 0.38, 0.22)

//////////////////////////////////////////////////////////////////////////////////////////
#ifndef SMAP_size
#define SMAP_size	1024.0
#endif
//////////////////////////////////////////////////////////////////////////////////////////

#endif	//	common_defines_h_included

#ifndef	common_policies_h_included
#define	common_policies_h_included

//	Define default sample index for MSAA
#ifndef	ISAMPLE
#define ISAMPLE 0
#endif	//	ISAMPLE

//	redefine sample index
#ifdef 	MSAA_OPTIMIZATION
#undef	ISAMPLE
#define ISAMPLE	iSample
#endif	//	MSAA_OPTIMIZATION

/////////////////////////////////////////////////////////////////////////////
// GLD_P - gbuffer_load_data
#ifdef	GBUFFER_OPTIMIZATION
	#define	GLD_P( _tc, _pos2d, _iSample ) _tc, _pos2d, _iSample
#else	//	GBUFFER_OPTIMIZATION
	#define	GLD_P( _tc, _pos2d, _iSample ) _tc, _iSample
#endif	//	GBUFFER_OPTIMIZATION

/////////////////////////////////////////////////////////////////////////////
// CS_P
#ifdef USE_MSAA
#	ifdef	GBUFFER_OPTIMIZATION
#		define	CS_P( _P, _N, _tc0, _tcJ, _pos2d, _iSample ) _P, _N, _tc0, _tcJ, _pos2d, _iSample
#	else	//	GBUFFER_OPTIMIZATION
#		define	CS_P( _P, _N, _tc0, _tcJ, _pos2d, _iSample ) _P, _N, _tc0, _tcJ, _iSample
#	endif	//	GBUFFER_OPTIMIZATION
#else
#	ifdef	GBUFFER_OPTIMIZATION
#		define	CS_P( _P, _N, _tc0, _tcJ, _pos2d, _iSample ) _P, _N, _tc0, _tcJ, _pos2d
#	else	//	GBUFFER_OPTIMIZATION
#		define	CS_P( _P, _N, _tc0, _tcJ, _pos2d, _iSample ) _P, _N, _tc0, _tcJ
#	endif
#endif

#endif	//	common_policies_h_included

#ifndef	common_iostructs_h_included
#define	common_iostructs_h_included

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

vec4	mul(int a,		vec4 b)	{ return float(a) * b; }
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
	float 	phase 	= 1.0/(2.0*3.141592653589);
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
 


////////////////////////////////////////////////////////////////
//	This file contains io structs:
//	v_name	:	input for vertex shader.
//	v2p_name:	output for vertex shader.
//	p_name	:	input for pixel shader.
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//	TL0uv
struct	v_TL0uv_positiont
{
	float4	P		; // POSITIONT;
	float4	Color	; // COLOR; 
};

struct	v_TL0uv
{
	float4	P		; // POSITION;
	float4	Color	; // COLOR; 
};

struct	v2p_TL0uv
{
	float4	Color	; // COLOR;
	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

struct	p_TL0uv
{
	float4	Color	; // COLOR;
//	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

////////////////////////////////////////////////////////////////
//	TL
struct	v_TL_positiont
{
	float4	P		; // POSITIONT;
	float2	Tex0	; // TEXCOORD0;
	float4	Color	; // COLOR; 
};

struct	v_TL
{
	float4	P		; // POSITION;
	float2	Tex0	; // TEXCOORD0;
	float4	Color	; // COLOR; 
};

struct	v2p_TL
{
	float2 	Tex0	; // TEXCOORD0;
	float4	Color	; // COLOR;
	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

struct	p_TL
{
	float2 	Tex0	; // TEXCOORD0;
	float4	Color	; // COLOR;
//	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

////////////////////////////////////////////////////////////////
//	TL2uv
struct	v_TL2uv
{
	float4	P		; // POSITIONT;
	float2	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float4	Color	; // COLOR; 
};

struct	v2p_TL2uv
{
	float2 	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float4	Color	; // COLOR;
	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

struct	p_TL2uv
{
	float2 	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float4	Color	; // COLOR;
//	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};
////////////////////////////////////////////////////////////////
//	postpr
struct	v_postpr
{
	float4	P		; // POSITIONT;
	float2 	Tex0	; // TEXCOORD0;	// base1 (duality)	
	float2	Tex1	; // TEXCOORD1;	// base2 (duality)
	float2	Tex2	; // TEXCOORD2;	// base  (noise)
	float4	Color	; // COLOR0;		// multiplier, color.w = noise_amount
	float4	Gray	; // COLOR1;		// (.3,.3,.3.,amount)
};

struct	v2p_postpr
{
	float2 	Tex0	; // TEXCOORD0;	// base1 (duality)	
	float2	Tex1	; // TEXCOORD1;	// base2 (duality)
	float2	Tex2	; // TEXCOORD2;	// base  (noise)
	float4	Color	; // COLOR0;		// multiplier, color.w = noise_amount
	float4	Gray	; // COLOR1;		// (.3,.3,.3.,amount)
	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

struct	p_postpr
{
	float2 	Tex0	; // TEXCOORD0;	// base1 (duality)	
	float2	Tex1	; // TEXCOORD1;	// base2 (duality)
	float2	Tex2	; // TEXCOORD2;	// base  (noise)
	float4	Color	; // COLOR0;		// multiplier, color.w = noise_amount
	float4	Gray	; // COLOR1;		// (.3,.3,.3.,amount)
//	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};
////////////////////////////////////////////////////////////////
//	build	(bloom_build)
struct	v_build
{
	float4	P		; // POSITIONT;
	float2	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float2 	Tex2	; // TEXCOORD2;
	float2	Tex3	; // TEXCOORD3;
};

struct	v2p_build
{
	float2 	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float2 	Tex2	; // TEXCOORD2;
	float2	Tex3	; // TEXCOORD3;
	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

struct	p_build
{
	float2 	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float2 	Tex2	; // TEXCOORD2;
	float2	Tex3	; // TEXCOORD3;
//	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};
////////////////////////////////////////////////////////////////
//	filter	(bloom_filter)
struct	v_filter
{
	float4	P		; // POSITIONT;
	float4 	Tex0	; // TEXCOORD0;
	float4	Tex1	; // TEXCOORD1;
	float4 	Tex2	; // TEXCOORD2;
	float4	Tex3	; // TEXCOORD3;
	float4 	Tex4	; // TEXCOORD4;
	float4	Tex5	; // TEXCOORD5;
	float4 	Tex6	; // TEXCOORD6;
	float4	Tex7	; // TEXCOORD7;
};

struct	v2p_filter
{
	float4 	Tex0	; // TEXCOORD0;
	float4	Tex1	; // TEXCOORD1;
	float4 	Tex2	; // TEXCOORD2;
	float4	Tex3	; // TEXCOORD3;
	float4 	Tex4	; // TEXCOORD4;
	float4	Tex5	; // TEXCOORD5;
	float4 	Tex6	; // TEXCOORD6;
	float4	Tex7	; // TEXCOORD7;
	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

struct	p_filter
{
	float4 	Tex0	; // TEXCOORD0;
	float4	Tex1	; // TEXCOORD1;
	float4 	Tex2	; // TEXCOORD2;
	float4	Tex3	; // TEXCOORD3;
	float4 	Tex4	; // TEXCOORD4;
	float4	Tex5	; // TEXCOORD5;
	float4 	Tex6	; // TEXCOORD6;
	float4	Tex7	; // TEXCOORD7;
//	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

////////////////////////////////////////////////////////////////
//	aa_AA
struct	v_aa_AA
{
	float4 P		; // POSITIONT;
	float2 	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float2 	Tex2	; // TEXCOORD2;
	float2	Tex3	; // TEXCOORD3;
	float2	Tex4	; // TEXCOORD4;
	float4	Tex5	; // TEXCOORD5;
	float4	Tex6	; // TEXCOORD6;
};

struct	v2p_aa_AA
{
	float2 	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float2 	Tex2	; // TEXCOORD2;
	float2	Tex3	; // TEXCOORD3;
	float2	Tex4	; // TEXCOORD4;
	float4	Tex5	; // TEXCOORD5;
	float4	Tex6	; // TEXCOORD6;
	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

struct	p_aa_AA
{
	float2 	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float2 	Tex2	; // TEXCOORD2;
	float2	Tex3	; // TEXCOORD3;
	float2	Tex4	; // TEXCOORD4;
	float4	Tex5	; // TEXCOORD5;
	float4	Tex6	; // TEXCOORD6;
//	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

struct	p_aa_AA_sun
{
	float2 	tc		; // TEXCOORD0;
	float2	unused	; // TEXCOORD1;
	float2 	LT		; // TEXCOORD2;
	float2	RT		; // TEXCOORD3;
	float2	LB		; // TEXCOORD4;
	float2	RB		; // TEXCOORD5;
//	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

////////////////////////////////////////////////////////////////
//	dumb
struct 	v_dumb
{
	float4	P		; // POSITION;	// Clip-space position 	(for rasterization)
};

struct 	v2p_dumb
{
	float4	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
};

////////////////////////////////////////////////////////////////
//	Volume
struct 	v2p_volume
{
	float4 	tc		; // TEXCOORD0;
#ifdef 	USE_SJITTER
	float4 	tcJ		; // TEXCOORD1;
#endif
	float4 	hpos	; // SV_Position;	// Clip-space position 	(for rasterization)
};
struct 	p_volume
{
	float4 	tc		; // TEXCOORD0;
#ifdef 	USE_SJITTER
	float4 	tcJ		; // TEXCOORD1;
#endif
//	float4 	hpos	; // SV_Position;	// Clip-space position 	(for rasterization)
};
////////////////////////////////////////////////////////////////
//	Static
struct         v_static
{
	float4	Nh		; // NORMAL;	// (nx,ny,nz,hemi occlusion)
	float4	T		; // TANGENT;	// tangent
	float4	B		; // BINORMAL;	// binormal
	float2	tc		; // TEXCOORD0;	// (u,v)
#ifdef	USE_LM_HEMI
	float2	lmh		; // TEXCOORD1;	// (lmu,lmv)
#endif
//	float4	color	; // COLOR0;	// (r,g,b,dir-occlusion)	//	Swizzle before use!!!
	float4	P		; // POSITION;	// (float,float,float,1)
};

struct	v_static_color
{
	float4	Nh		; // NORMAL;	// (nx,ny,nz,hemi occlusion)
	float4	T		; // TANGENT;	// tangent
	float4	B		; // BINORMAL;	// binormal
	float2	tc		; // TEXCOORD0;	// (u,v)
#ifdef	USE_LM_HEMI
	float2	lmh		; // TEXCOORD1;	// (lmu,lmv)
#endif
	float4	color	; // COLOR0;	// (r,g,b,dir-occlusion)	//	Swizzle before use!!!
	float4	P		; // POSITION;	// (float,float,float,1)
};

////////////////////////////////////////////////////////////////
//	defer
#ifndef GBUFFER_OPTIMIZATION
struct                  f_deffer        		
{
	float4	position; // SV_Target0;        // px,py,pz, m-id
	float4	Ne		  ; // SV_Target1;        // nx,ny,nz, hemi
	float4	C		  ; // SV_Target2;        // r, g, b,  gloss
#ifdef EXTEND_F_DEFFER
   uint     mask    ; // SV_COVERAGE;
#endif
};
#else
struct                  f_deffer        		
{
	float4	position; // SV_Target0;        // xy=encoded normal, z = pz, w = encoded(m-id,hemi)
	float4	C		  ; // SV_Target1;        // r, g, b,  gloss
#ifdef EXTEND_F_DEFFER
   uint     mask    ; // SV_COVERAGE;
#endif
};
#endif

struct					gbuffer_data
{
	float3  P; // position.( mtl or sun )
	float   mtl; // material id
	float3  N; // normal
	float   hemi; // AO
	float3  C;
	float   gloss;
};

////////////////////////////////////////////////////////////////
//	Defer bumped
struct v2p_bumped
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	float4	tcdh	; // TEXCOORD0;	// Texture coordinates,         w=sun_occlusion
#else
	float2	tcdh	; // TEXCOORD0;	// Texture coordinates
#endif
	float4	position; // TEXCOORD1;	// position + hemi
	float3	M1		; // TEXCOORD2;	// nmap 2 eye - 1
	float3	M2		; // TEXCOORD3;	// nmap 2 eye - 2
	float3	M3		; // TEXCOORD4;	// nmap 2 eye - 3
#ifdef USE_TDETAIL
	float2	tcdbump	; // TEXCOORD5;	// d-bump
#endif
#ifdef USE_LM_HEMI
		float2	lmh	; // TEXCOORD6;	// lm-hemi
#endif
	float4	hpos	; // SV_Position;
};

struct p_bumped
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	float4	tcdh	; // TEXCOORD0;	// Texture coordinates,         w=sun_occlusion
#else
	float2	tcdh	; // TEXCOORD0;	// Texture coordinates
#endif
	float4	position; // TEXCOORD1;	// position + hemi
	float3	M1		; // TEXCOORD2;	// nmap 2 eye - 1
	float3	M2		; // TEXCOORD3;	// nmap 2 eye - 2
	float3	M3		; // TEXCOORD4;	// nmap 2 eye - 3
#ifdef USE_TDETAIL
	float2	tcdbump	; // TEXCOORD5;	// d-bump
#endif
#ifdef USE_LM_HEMI
		float2	lmh	; // TEXCOORD6;	// lm-hemi
#endif
};
////////////////////////////////////////////////////////////////
//	Defer flat
struct	v2p_flat
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	float4	tcdh	; // TEXCOORD0;	// Texture coordinates,         w=sun_occlusion
#else
	float2	tcdh	; // TEXCOORD0;	// Texture coordinates
#endif
	float4	position; // TEXCOORD1;	// position + hemi
	float3	N		; // TEXCOORD2;	// Eye-space normal        (for lighting)
#ifdef USE_TDETAIL
	float2	tcdbump	; // TEXCOORD3;	// d-bump
#endif
#ifdef USE_LM_HEMI
	float2	lmh		; // TEXCOORD4;	// lm-hemi
#endif
	float4	hpos	; // SV_Position;
};

struct	p_flat
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	float4	tcdh	; // TEXCOORD0;	// Texture coordinates,         w=sun_occlusion
#else
	float2	tcdh	; // TEXCOORD0;	// Texture coordinates
#endif
	float4	position; // TEXCOORD1;	// position + hemi
	float3	N		; // TEXCOORD2;	// Eye-space normal        (for lighting)
#ifdef USE_TDETAIL
	float2	tcdbump	; // TEXCOORD3;	// d-bump
#endif
#ifdef USE_LM_HEMI
	float2	lmh		; // TEXCOORD4;	// lm-hemi
#endif
};

////////////////////////////////////////////////////////////////
//	Shadow
struct	v_shadow_direct_aref
{
	float4	P		; // POSITION;		// (float,float,float,1)
	float4	tc		; // TEXCOORD0;	// (u,v,frac,???)
};

struct	v_shadow_direct
{
	float4	P		; // POSITION;		// (float,float,float,1)
};


struct	v2p_shadow_direct_aref
{
	float2	tc0		; // TEXCOORD1;	// Diffuse map for aref
	float4	hpos	; // SV_Position;	// Clip-space position         (for rasterization)
};

struct	v2p_shadow_direct
{
	float4	hpos	; // SV_Position;		// Clip-space position         (for rasterization)
};

struct	p_shadow_direct_aref
{
	float2	tc0		; // TEXCOORD1;	// Diffuse map for aref
};

////////////////////////////////////////////////////////////////
//	Model
struct	v_model
{
	float4	P		; // POSITION;		// (float,float,float,1)
	float3	N		; // NORMAL;		// (nx,ny,nz)
	float3	T		; // TANGENT;		// (nx,ny,nz)
	float3	B		; // BINORMAL;		// (nx,ny,nz)
	float2	tc		; // TEXCOORD0;	// (u,v)
};

////////////////////////////////////////////////////////////////
//	Tree
struct	v_tree
{
	float4	P		; // POSITION;		// (float,float,float,1)
	float4	Nh		; // NORMAL;		// (nx,ny,nz)
	float3	T		; // TANGENT;		// tangent
	float3	B		; // BINORMAL;		// binormal
	float4	tc		; // TEXCOORD0;	// (u,v,frac,???)
};

////////////////////////////////////////////////////////////////
//	Details
struct        v_detail                    
{
        float4      pos			; // POSITION;                // (float,float,float,1)
        float4      misc		; // TEXCOORD0;        // (u(Q),v(Q),frac,matrix-id)
};

#endif	//	common_iostructs_h_included

#ifndef	common_samplers_h_included
#define	common_samplers_h_included

#define Texture2D	uniform sampler2D
#define Texture3D	uniform sampler3D
#define Texture2DMS uniform sampler2DMS
#define TextureCube uniform samplerCube
#define Texture2DShadow uniform sampler2DShadow

//////////////////////////////////////////////////////////////////////////////////////////
// Geometry phase / deferring               	//

//sampler 	smp_nofilter;   //	Use D3DTADDRESS_CLAMP,	D3DTEXF_POINT,			D3DTEXF_NONE,	D3DTEXF_POINT 
//sampler 	smp_rtlinear;	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR 
//sampler 	smp_linear;		//	Use	D3DTADDRESS_WRAP,	D3DTEXF_LINEAR,			D3DTEXF_LINEAR,	D3DTEXF_LINEAR
//sampler 	smp_base;		//	Use D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC, 	D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC

Texture2D 	s_base;		//	smp_base
#ifdef USE_MSAA
Texture2DMS	s_generic;	//	smp_generic
#else
Texture2D   s_generic;
#endif
Texture2D 	s_bump;             	//
Texture2D 	s_bumpX;                //
Texture2D 	s_detail;               //
Texture2D 	s_detailBump;           //	
Texture2D 	s_detailBumpX;          //	Error for bump detail
//Texture2D 	s_bumpD;                //
Texture2D 	s_hemi;             	//

Texture2D 	s_mask;             	//

Texture2D 	s_dt_r;                	//
Texture2D 	s_dt_g;                	//
Texture2D 	s_dt_b;                	//
Texture2D 	s_dt_a;                	//

Texture2D 	s_dn_r;                	//
Texture2D 	s_dn_g;                	//
Texture2D 	s_dn_b;                	//
Texture2D 	s_dn_a;                	//

//////////////////////////////////////////////////////////////////////////////////////////
// Lighting/shadowing phase                     //

//sampler 	smp_material;

//uniform sampler2D       s_depth;                //
#ifdef USE_MSAA
Texture2DMS	s_position;	//	smp_nofilter or Load
Texture2DMS	s_normal;	//	smp_nofilter or Load
#else
Texture2D	s_position;	//	smp_nofilter or Load
Texture2D	s_normal;	//	smp_nofilter or Load
#endif
Texture2D	s_lmap;		// 2D/???cube projector lightmap
Texture3D	s_material;	//	smp_material
//uniform sampler1D       s_attenuate;        	//


//////////////////////////////////////////////////////////////////////////////////////////
// Combine phase                                //
#ifdef USE_MSAA
Texture2DMS	s_diffuse;	// rgb.a = diffuse.gloss
Texture2DMS	s_accumulator;      	// rgb.a = diffuse.specular
#else
Texture2D	s_diffuse;	// rgb.a = diffuse.gloss
Texture2D	s_accumulator;      	// rgb.a = diffuse.specular
#endif
//uniform sampler2D       s_generic;              //
Texture2D	s_bloom;	//
Texture2D	s_image;	// used in various post-processing
Texture2D	s_tonemap;	// actually MidleGray / exp(Lw + eps)


#endif	//	#ifndef	common_samplers_h_included

#ifndef	common_cbuffers_h_included
#define	common_cbuffers_h_included

#ifndef MSAA_OPTIMIZATION
//	Used by dynamic lights and volumetric effects
// TODO: OGL: Use constant buffers.
//cbuffer	dynamic_light
//{
	uniform float4	Ldynamic_color;	// dynamic light color (rgb1)	- spot/point/sun
	uniform float4	Ldynamic_pos;	// dynamic light pos+1/range(w)	- spot/point
	uniform float4	Ldynamic_dir;	// dynamic light direction		- sun
//}
#else
//cbuffer	dynamic_light
//{
	uniform float4	Ldynamic_color;	// dynamic light color (rgb1)	- spot/point/sun
	uniform float4	Ldynamic_pos;	// dynamic light pos+1/range(w)	- spot/point
	uniform float4	Ldynamic_dir;	// dynamic light direction		- sun
//}
#endif

#endif	//	common_cbuffers_h_included

#ifndef	common_functions_h_included
#define	common_functions_h_included

//	contrast function
float Contrast(float Input, float ContrastPower)
{
     //piecewise contrast function
     bool IsAboveHalf = Input > 0.5 ;
     float ToRaise = saturate(2.0*(IsAboveHalf ? (1.0 - Input) : Input));
     float Output = 0.5*pow(ToRaise, ContrastPower); 
     Output = IsAboveHalf ? (1.0 - Output) : Output;
     return Output;
}

#ifdef USE_SHOC_RESOURCES
void tonemap( out float4 low, out float4 high, float3 rgb, float scale)
{
        rgb	= rgb*scale;

        low	= rgb.xyzz;
        high	= low/def_hdr;        // 8x dynamic range
}
#else // USE_SHOC_RESOURCES
void tonemap( out float4 low, out float4 high, float3 rgb, float scale)
{
	rgb		= rgb*scale;

	const float fWhiteIntensity = 1.7;
	const float fWhiteIntensitySQR = fWhiteIntensity*fWhiteIntensity;

	low	= ( (rgb*(1.0+rgb/fWhiteIntensitySQR)) / (rgb+1.0) ).xyzz;
	high	= rgb.xyzz/def_hdr;	// 8x dynamic range
}
#endif // USE_SHOC_RESOURCES

float4 combine_bloom( float3  low, float4 high)	
{
	return float4( low + high.rgb*high.a, 1.0 );
}

float calc_fogging( float4 w_pos )      
{
	return dot(w_pos,fog_plane);         
}

float2 unpack_tc_base( float2 tc, float du, float dv )
{
	return (tc.xy + float2	(du,dv))*(32.0/32768.0); //!Increase from 32bit to 64bit floating point
}

float3 calc_sun_r1( float3 norm_w )    
{
	return L_sun_color*saturate(dot((norm_w),-L_sun_dir_w));                 
}

float3 calc_model_hemi_r1( float3 norm_w )    
{
	return max(0.0,norm_w.y)*L_hemi_color.rgb;
}

float3 calc_model_lq_lighting( float3 norm_w )    
{
	return L_material.x*calc_model_hemi_r1(norm_w) + L_ambient.rgb + L_material.y*calc_sun_r1(norm_w);
}

float3 	unpack_normal( float3 v )	{ return 2.0*v-1.0; }
float3 	unpack_normal( float4 v )	{ return 2.0*v.xyz-1.0; }
float3 	unpack_bx2( float3 v )	{ return 2.0*v-1.0; }
float3 	unpack_bx2( float4 v )	{ return 2.0*v.xyz-1.0; }
float3 	unpack_bx4( float3 v )	{ return 4.0*v-2.0; } //!reduce the amount of stretching from 4*v-2 and increase precision
float3 	unpack_bx4( float4 v )	{ return 4.0*v.xyz-2.0; }
float2 	unpack_tc_lmap( float2 tc )	{ return tc*(1.0/32768.0);	} // [-1  .. +1 ] 
float4	unpack_color( float4 c ) { return c.bgra; }
float4	unpack_D3DCOLOR( float4 c ) { return c.bgra; }
float3	unpack_D3DCOLOR( float3 c ) { return c.bgr; }

float3   p_hemi(float2 tc)
{
	float4 t_lmh = tex2D(s_hemi, tc);

#ifdef USE_SHOC_RESOURCES
	float r_lmh = (1.0/3.0);
	return float3(dot(t_lmh.rgb, float3(r_lmh, r_lmh, r_lmh)));
#else // USE_SHOC_RESOURCES
	return float3(t_lmh.a);
#endif // USE_SHOC_RESOURCES
}

float	get_hemi(float4 lmh)
{
#ifdef USE_SHOC_RESOURCES
	float r_lmh = (1.0/3.0);
	return dot(lmh.rgb, float3(r_lmh, r_lmh, r_lmh));
#else // USE_SHOC_RESOURCES
	return lmh.a;
#endif // USE_SHOC_RESOURCES
}

float	get_sun(float4 lmh)
{
#ifdef USE_SHOC_RESOURCES
	return lmh.a;
#else // USE_SHOC_RESOURCES
	return lmh.g;
#endif // USE_SHOC_RESOURCES
}

float3	v_hemi(float3 n)
{
	return L_hemi_color.rgb*(0.5 + 0.5*n.y);                   
}

float3	v_sun(float3 n)                        	
{
	return L_sun_color*dot(n,-L_sun_dir_w);                
}

float3	calc_reflection( float3 pos_w, float3 norm_w )
{
    return reflect(normalize(pos_w-eye_position), norm_w);
}

#define USABLE_BIT_1                uint(0x00002000)
#define USABLE_BIT_2                uint(0x00004000)
#define USABLE_BIT_3                uint(0x00008000)
#define USABLE_BIT_4                uint(0x00010000)
#define USABLE_BIT_5                uint(0x00020000)
#define USABLE_BIT_6                uint(0x00040000)
#define USABLE_BIT_7                uint(0x00080000)
#define USABLE_BIT_8                uint(0x00100000)
#define USABLE_BIT_9                uint(0x00200000)
#define USABLE_BIT_10               uint(0x00400000)
#define USABLE_BIT_11               uint(0x00800000)   // At least two of those four bit flags must be mutually exclusive (i.e. all 4 bits must not be set together)
#define USABLE_BIT_12               uint(0x01000000)   // This is because setting 0x47800000 sets all 5 FP16 exponent bits to 1 which means infinity
#define USABLE_BIT_13               uint(0x02000000)   // This will be translated to a +/-MAX_FLOAT in the FP16 render target (0xFBFF/0x7BFF), overwriting the 
#define USABLE_BIT_14               uint(0x04000000)   // mantissa bits where other bit flags are stored.
#define USABLE_BIT_15               uint(   1 << 31)   // = uint(0x80000000) // fix for integrated Intel cards
#define MUST_BE_SET                 uint(0x40000000)   // This flag *must* be stored in the floating-point representation of the bit flag to store

/*
float2 gbuf_pack_normal( float3 norm )
{
   float2 res;

   res = 0.5 * ( norm.xy + float2( 1, 1 ) ) ;
   res.x *= ( norm.z < 0 ? -1.0 : 1.0 );

   return res;
}

float3 gbuf_unpack_normal( float2 norm )
{
   float3 res;

   res.xy = ( 2.0 * abs( norm ) ) - float2(1,1);

   res.z = ( norm.x < 0 ? -1.0 : 1.0 ) * sqrt( abs( 1 - res.x * res.x - res.y * res.y ) );

   return res;
}
*/

// Holger Gruen AMD - I change normal packing and unpacking to make sure N.z is accessible without ALU cost
// this help the HDAO compute shader to run more efficiently
float2 gbuf_pack_normal( float3 norm )
{
   float2 res;

   res.x  = norm.z;
   res.y  = 0.5 * ( norm.x + 1.0 ) ;
   res.y *= ( norm.y < 0.0 ? -1.0 : 1.0 );

   return res;
}

float3 gbuf_unpack_normal( float2 norm )
{
   float3 res;

   res.z  = norm.x;
   res.x  = ( 2.0 * abs( norm.y ) ) - 1.0;
   res.y = ( norm.y < 0.0 ? -1.0 : 1.0 ) * sqrt( abs( 1.0 - res.x * res.x - res.z * res.z ) );

   return res;
}

float gbuf_pack_hemi_mtl( float hemi, float mtl )
{
   uint packed_mtl = uint( ( mtl / 1.333333333 ) * 31.0 );
//   uint packed_hemi = ( MUST_BE_SET + ( uint( hemi * 255.0 ) << 13 ) + ( ( packed_mtl & uint( 31 ) ) << 21 ) );
	//	Clamp hemi max value
	uint packed_hemi = ( MUST_BE_SET + ( uint( saturate(hemi) * 255.9 ) << 13 ) + ( ( packed_mtl & uint( 31 ) ) << 21 ) );

   if( ( packed_hemi & USABLE_BIT_13 ) == 0u )
      packed_hemi |= USABLE_BIT_14;

   if( ( packed_mtl & uint( 16 ) ) != 0u )
      packed_hemi |= USABLE_BIT_15;

   return asfloat( packed_hemi );
}

float gbuf_unpack_hemi( float mtl_hemi )
{
//   return float( ( asuint( mtl_hemi ) >> 13u ) & uint(255) ) * (1.0/255.0);
	return float( ( asuint( mtl_hemi ) >> 13u ) & uint(255) ) * (1.0/254.8);
}

float gbuf_unpack_mtl( float mtl_hemi )
{
   uint packed_mtl       = asuint( mtl_hemi );
   uint packed_hemi  = ( ( packed_mtl >> 21u ) & 15u ) + ( ( packed_mtl & USABLE_BIT_15 ) == 0u ? 0u : 16u );
   return float( packed_hemi ) * (1.0/31.0) * 1.333333333;
}

#ifndef EXTEND_F_DEFFER
f_deffer pack_gbuffer( float4 norm, float4 pos, float4 col )
#else
f_deffer pack_gbuffer( float4 norm, float4 pos, float4 col, uint imask )
#endif
{
	f_deffer res;

#ifndef GBUFFER_OPTIMIZATION
	res.position	= pos;
	res.Ne			= norm;
	res.C			   = col;
#else
	res.position	= float4( gbuf_pack_normal( norm.xyz ), pos.z, gbuf_pack_hemi_mtl( norm.w, pos.w ) );
	res.C			   = col;
#endif

#ifdef EXTEND_F_DEFFER
   res.mask = imask;
#endif

	return res;
}

#ifdef GBUFFER_OPTIMIZATION
gbuffer_data gbuffer_load_data( float2 tc, float4 pos2d, uint iSample )
{
	gbuffer_data gbd;

	gbd.P = float3(0.0,0.0,0.0);
	gbd.hemi = 0.0;
	gbd.mtl = 0.0;
	gbd.C = float3(0.0,0.0,0.0);
	gbd.N = float3(0.0,0.0,0.0);

#ifndef USE_MSAA
	float4 P	= tex2D( s_position, tc );
#else
	float4 P	= texelFetch( s_position, int2( pos2d ), int(iSample) );
#endif

	// 3d view space pos reconstruction math
	// center of the plane (0,0) or (0.5,0.5) at distance 1 is eyepoint(0,0,0) + lookat (assuming |lookat| ==1
	// left/right = (0,0,1) -/+ tan(fHorzFOV/2) * (1,0,0 ) 
	// top/bottom = (0,0,1) +/- tan(fVertFOV/2) * (0,1,0 )
	// lefttop		= ( -tan(fHorzFOV/2),  tan(fVertFOV/2), 1 )
	// righttop		= (  tan(fHorzFOV/2),  tan(fVertFOV/2), 1 )
	// leftbottom   = ( -tan(fHorzFOV/2), -tan(fVertFOV/2), 1 )
	// rightbottom	= (  tan(fHorzFOV/2), -tan(fVertFOV/2), 1 )
	gbd.P  = float3( P.z * ( pos2d.xy * pos_decompression_params.zw - pos_decompression_params.xy ), P.z );

	// reconstruct N
	gbd.N = gbuf_unpack_normal( P.xy );

	// reconstruct material
	gbd.mtl	= gbuf_unpack_mtl( P.w );

   // reconstruct hemi
   gbd.hemi = gbuf_unpack_hemi( P.w );

#ifndef USE_MSAA
   float4	C	= tex2D( s_diffuse, tc );
#else
   float4	C	= texelFetch( s_diffuse, int2( pos2d ), int(iSample) );
#endif

	gbd.C		= C.xyz;
	gbd.gloss	= C.w;

	return gbd;
}

gbuffer_data gbuffer_load_data( float2 tc, float4 pos2d )
{
   return gbuffer_load_data( tc, pos2d, 0u );
}

gbuffer_data gbuffer_load_data_offset( float2 tc, float2 OffsetTC, float4 pos2d )
{
	float4  delta	  = float4( ( OffsetTC - tc ) * pos_decompression_params2.xy, 0, 0 );

	return gbuffer_load_data( OffsetTC, pos2d + delta, 0u );
}

gbuffer_data gbuffer_load_data_offset( float2 tc, float2 OffsetTC, float4 pos2d, uint iSample )
{
   float4  delta	  = float4( ( OffsetTC - tc ) * pos_decompression_params2.xy, 0, 0 );

   return gbuffer_load_data( OffsetTC, pos2d + delta, iSample );
}

#else // GBUFFER_OPTIMIZATION

gbuffer_data gbuffer_load_data( float2 tc, uint iSample )
{
	gbuffer_data gbd;

#ifndef USE_MSAA
	float4 P	= tex2D( s_position, tc );
#else
   float4 P		= texelFetch( s_position, int2( tc * pos_decompression_params2.xy ), int(iSample) );
#endif

	gbd.P		= P.xyz;
	gbd.mtl		= P.w;

#ifndef USE_MSAA
	float4 N	= tex2D( s_normal, tc );
#else
	float4 N	= texelFetch( s_normal, int2( tc * pos_decompression_params2.xy ), int(iSample) );
#endif

	gbd.N		= N.xyz;
	gbd.hemi	= N.w;

#ifndef USE_MSAA
	float4	C	= tex2D( s_diffuse, tc );
#else
	float4	C	= texelFetch( s_diffuse, int2( tc * pos_decompression_params2.xy ), int(iSample) );
#endif


	gbd.C		= C.xyz;
	gbd.gloss	= C.w;

	return gbd;
}

gbuffer_data gbuffer_load_data( float2 tc  )
{
   return gbuffer_load_data( tc, 0u );
}

gbuffer_data gbuffer_load_data_offset( float2 tc, float2 OffsetTC, uint iSample )
{
   return gbuffer_load_data( OffsetTC, uint(iSample) );
}

#endif // GBUFFER_OPTIMIZATION

//////////////////////////////////////////////////////////////////////////
//	Aplha to coverage code
#if ( defined( MSAA_ALPHATEST_DX10_1_ATOC ) || defined( MSAA_ALPHATEST_DX10_1 ) )

#if MSAA_SAMPLES == 2
uint alpha_to_coverage ( float alpha, float2 pos2d )
{
	uint mask;
	uint pos = uint(pos2d.x) | uint( pos2d.y);
	if( alpha < 0.3333 )
		mask = 0;
	else if( alpha < 0.6666 )
		mask = 1 << ( pos & 1 );
	else 
		mask = 3;

	return mask;
}
#endif

#if MSAA_SAMPLES == 4
uint alpha_to_coverage ( float alpha, float2 pos2d )
{
	uint mask;

	float off = float( ( uint(pos2d.x) | uint( pos2d.y) ) & 3 );
	alpha = saturate( alpha - off * ( ( 0.2 / 4.0 ) / 3.0 ) );
	if( alpha < 0.40 )
	{
		if( alpha < 0.20 )
			mask = 0;	
		else if( alpha < 0.40 ) // only one bit set
			mask = 1;
	}
  else
  {
	if( alpha < 0.60 ) // 2 bits set => 1100 0110 0011 1001 1010 0101
	{
		mask = 3;
	}
	else if( alpha < 0.8 ) // 3 bits set => 1110 0111 1011 1101 
	  mask = 7;
	else
	  mask = 0xf;
 }

	return mask;
}
#endif

#if MSAA_SAMPLES == 8
uint alpha_to_coverage ( float alpha, float2 pos2d )
{
	uint mask;

	float off = float( ( uint(pos2d.x) | uint( pos2d.y) ) & 3 );
	alpha = saturate( alpha - off * ( ( 0.1111 / 8.0 ) / 3.0 ) );
  if( alpha < 0.4444 )
  {
	if( alpha < 0.2222 )
	{
		if( alpha < 0.1111 )
			mask = 0;	
		else // only one bit set 0.2222
			mask = 1;
	}
	else 
	{
		if( alpha < 0.3333 ) // 2 bits set0=> 10000001 + 11000000 .. 00000011 : 8 // 0.2222
		  				   //        set1=> 10100000 .. 00000101 + 10000010 + 01000001 : 8
						   //		set2=> 10010000 .. 00001001 + 10000100 + 01000010 + 00100001 : 8
						   //		set3=> 10001000 .. 00010001 + 10001000 + 01000100 + 00100010 + 00010001 : 8
		{  
			mask = 3;
		}
	    else // 3 bits set0 => 11100000 .. 00000111 + 10000011 + 11000001 : 8 ? 0.4444 // 0.3333
			 //        set1 => 10110000 .. 00001011 + 10000101 + 11000010 + 01100001: 8
			 //        set2 => 11010000 .. 00001101 + 10000110 + 01000011 + 10100001: 8
			 //        set3 => 10011000 .. 00010011 + 10001001 + 11000100 + 01100010 + 00110001 : 8
			 //        set4 => 11001000 .. 00011001 + 10001100 + 01000110 + 00100011 + 10010001 : 8
		{
			mask = 0x7;
		}
	}
  }
  else
  {
	  if( alpha < 0.6666 )
	  {
		if( alpha < 0.5555 ) // 4 bits set0 => 11110000 .. 00001111 + 10000111 + 11000011 + 11100001 : 8 // 0.5555
		 				   //        set1 => 11011000 .. 00011011 + 10001101 + 11000110 + 01100011 + 10110001 : 8
						   //        set2 => 11001100 .. 00110011 + 10011001 : 4 make 8
						   //        set3 => 11000110 + 01100011 + 10110001 + 11011000 + 01101100 + 00110110 + 00011011 + 10001101 : 8
						   //        set4 => 10111000 .. 00010111 + 10001011 + 11000101 + 11100010 + 01110001 : 8
						   //        set5 => 10011100 .. 00100111 + 10010011 + 11001001 + 11100100 + 01110010 + 00111001 : 8
						   //        set6 => 10101010 .. 01010101 : 2 make 8
						   //        set7 => 10110100 +  01011010 + 00101101 + 10010110 + 01001011 + 10100101 + 11010010 + 01101001 : 8
						   //        set8 => 10011010 +  01001101 + 10100110 + 01010011 + 10101001 + 11010100 + 01101010 + 00110101 : 8
		{
			mask = 0xf;
		}
		else // 5 bits set0 => 11111000 01111100 00111110 00011111 10001111 11000111 11100011 11110001 : 8  // 0.6666
		     //        set1 => 10111100 : 8
		     //        set2 => 10011110 : 8
		     //        set3 => 11011100 : 8
		     //        set4 => 11001110 : 8
		     //        set5 => 11011010 : 8
		     //        set6 => 10110110 : 8
		{
			mask = 0x1F;
		}
	  }
	  else
	  {
		if( alpha < 0.7777 ) // 6 bits set0 => 11111100 01111110 00111111 10011111 11001111 11100111 11110011 11111001 : 8
						  //        set1 => 10111110 : 8
						  //        set2 => 11011110 : 8
		{
			mask = 0x3F;
		}
		else if( alpha < 0.8888 ) // 7 bits set0 => 11111110 :8
		{
			mask = 0x7F;
		}
		else // all 8 bits set
			mask = 0xFF;
	 }
  }

	return mask;
}
#endif
#endif



#endif	//	common_functions_h_included
 


// #define USE_SUPER_SPECULAR

#ifdef        USE_R2_STATIC_SUN
#  define xmaterial float(1.0/4.0)
#else
#  define xmaterial float(L_material.w)
#endif

#endif
 

#ifndef _WATERCONFIG_H
#define _WATERCONFIG_H

//настройки для: (1)
//waterdistortion
//waterdistortion2

#define W_POSITION_SHIFT_HEIGHT (1.0/60.0)	//(1.0/100.0)	//amplitude /50 - small, /10 - large
#define W_POSITION_SHIFT_SPEED	(25.0)		//(25.0)
#define W_DISTORT_BASE_TILE_0	(1.0)		//(1.0)
#define W_DISTORT_BASE_TILE_1	(1.1)		//(1.1)
#define W_DISTORT_AMP_0		(+0.15)		//(+0.15)
#define W_DISTORT_AMP_1		(+0.55)		//(-0.30)
#define W_DISTORT_POWER		(1.0)		//(1.0)

#endif


/*
////////////////////////////////////////////////////////////////////////////////
-- waters clear
////////////////////////////////////////////////////////////////////////////////
настройки для:
	waterdistortion
	waterdistortion2
////////////////////////////////////////////////////////////////////////////////
#define W_POSITION_SHIFT_HEIGHT (1.0/50.0)	//(1.0/100.0)	//amplitude /50 - small, /10 - large
#define W_POSITION_SHIFT_SPEED	(15.0)		//(25.0)
#define W_DISTORT_BASE_TILE_0	(1.3)		//(1.6)
#define W_DISTORT_BASE_TILE_1	(2.3)		//(1.1)
#define W_DISTORT_AMP_0		(+0.35)		//(+0.15)
#define W_DISTORT_AMP_1		(-1.75)		//(-0.30)
#define W_DISTORT_POWER		(1.0)		//(1.0)
////////////////////////////////////////////////////////////////////////////////
настройки для:
	waterdistortion
	waterdistortion
////////////////////////////////////////////////////////////////////////////////
#define W_POSITION_SHIFT_HEIGHT (1.0/80.0)	//(1.0/100.0)	//amplitude /50 - small, /10 - large
#define W_POSITION_SHIFT_SPEED	(20.0)		//(25.0)
#define W_DISTORT_BASE_TILE_0	(1.6)		//(1.6)
#define W_DISTORT_BASE_TILE_1	(1.1)		//(1.1)
#define W_DISTORT_AMP_0		(+0.15)		//(+0.15)
#define W_DISTORT_AMP_1		(-0.30)		//(-0.30)
#define W_DISTORT_POWER		(6.0)		//(6.0)
////////////////////////////////////////////////////////////////////////////////
-- waters mulyaka
////////////////////////////////////////////////////////////////////////////////
#define W_POSITION_SHIFT_HEIGHT (1.0/50.0)	// amplitude /50 - small, /10 - large
#define W_POSITION_SHIFT_SPEED	(25.0)
#define W_DISTORT_BASE_TILE	(0.1)		//(1.0)
#define W_DISTORT_AMP_0		(+0.58)		//(-0.08)
#define W_DISTORT_AMP_1		(+0.38)		//(+0.18)
#define W_DISTORT_POWER		(3.0)		//(2.0)
////////////////////////////////////////////////////////////////////////////////
*/
 

#ifndef _WATERMOVE_H
#define _WATERMOVE_H

float4	watermove	(float4 P)	{
	float3 	wave1	= float3(0.11, 0.13, 0.07)*W_POSITION_SHIFT_SPEED	;
	float 	dh	= sin  	(timers.x+dot(P.xyz,wave1))			;
			P.y	+= dh * W_POSITION_SHIFT_HEIGHT	;
	return 	P	;
}
float2	watermove_tc	(float2 base, float2 P, float amp)	{
	float2 	wave1	= 	float2	(0.2111,0.2333)*amp	;
	float 	angle 	= 	timers.z + dot (P,wave1)	;
	float 	du	= 	sin  	(angle);
	float 	dv	= 	cos  	(angle);
		return	(base + amp*float2(du,dv));
}

float3	waterrefl	(out float amount, float3 P, float3 N)	{
	float3 	v2point	= normalize	(P-eye_position);
	float3	vreflect= reflect	(v2point, N);
	float 	fresnel	= (0.5 + 0.5*dot(vreflect,v2point));
			amount	= 1 - fresnel*fresnel;			// 0=full env, 1=no env
	return	vreflect;
}

#endif
 


out gl_PerVertex { vec4 gl_Position; };

struct v_vert
{
	float4 	P	;	// POSITION;		// (float,float,float,1)
	float4	N	;	// NORMAL;		// (nx,ny,nz,hemi occlusion)
	float4 	T	;	// TANGENT;
	float4 	B	;	// BINORMAL;
	float4	color	;	// COLOR0;		// (r,g,b,dir-occlusion)
	float2 	uv	;	// TEXCOORD0;		// (u0,v0)
};
struct v2p
{
	float4	hpos	;	// SV_Position;
	float2	tbase	;	// TEXCOORD0;		// base
	float2	tnorm0	;	// TEXCOORD1;		// nm0
	float2	tnorm1	;	// TEXCOORD2;		// nm1
	float3	M1	;	// TEXCOORD3;
	float3	M2	;	// TEXCOORD4;
	float3	M3	;	// TEXCOORD5;
	float3	v2point	;	// TEXCOORD6;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	float4	tctexgen;	// TEXCOORD7;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
#if SSR_QUALITY > 0
	float4	position_w;	// POSITION0;
#endif
	float4	c0	;	// COLOR0;
	float	fog	;	// FOG;
};

layout(location = POSITION)		in float4	v_vert_P		; // POSITION;		// (float,float,float,1)
layout(location = NORMAL)		in float4	v_vert_N		; // NORMAL;		// (nx,ny,nz,hemi occlusion)
layout(location = TANGENT)		in float4	v_vert_T		; // TANGENT;		// tangent
layout(location = BINORMAL)		in float4	v_vert_B		; // BINORMAL;		// binormal
layout(location = COLOR0)		in float4	v_vert_color		; // COLOR0;		// (r,g,b,dir-occlusion)
layout(location = TEXCOORD0)		in float2	v_vert_uv		; // TEXCOORD0;		// (u0,v0)

layout(location = TEXCOORD0) 		out float2	v2p_vert_tbase		; // TEXCOORD0;
layout(location = TEXCOORD1) 		out float2	v2p_vert_tnorm0		; // TEXCOORD1;
layout(location = TEXCOORD2) 		out float2	v2p_vert_tnorm1		; // TEXCOORD2;
layout(location = TEXCOORD3) 		out float3	v2p_vert_M1		; // TEXCOORD3;
layout(location = TEXCOORD4) 		out float3	v2p_vert_M2		; // TEXCOORD4;
layout(location = TEXCOORD5) 		out float3	v2p_vert_M3		; // TEXCOORD5;
layout(location = TEXCOORD6) 		out float3	v2p_vert_v2point	; // TEXCOORD6;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
layout(location = TEXCOORD7) 		out float4	v2p_vert_tctexgen	; // TEXCOORD7;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
#if SSR_QUALITY > 0
layout(location = POSITION0) 		out float4	v2p_vert_pos	; // POSITION0;
#endif
layout(location = COLOR0) 		out float4	v2p_vert_c0		; // COLOR0;
layout(location = FOG) 			out float	v2p_vert_fog		; // FOG;

v2p _main (v_vert v);

void main()
{
	v_vert		I;
	I.P		= v_vert_P;
	I.N		= v_vert_N;
	I.T		= v_vert_T;
	I.B		= v_vert_B;
	I.color		= v_vert_color;
	I.uv		= v_vert_uv;

	v2p O 		= _main (I);

	v2p_vert_tbase	= O.tbase;
	v2p_vert_tnorm0	= O.tnorm0;
	v2p_vert_tnorm1	= O.tnorm1;
	v2p_vert_M1	= O.M1;
	v2p_vert_M2	= O.M2;
	v2p_vert_M3	= O.M3;
	v2p_vert_v2point= O.v2point;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	v2p_vert_tctexgen = O.tctexgen;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	v2p_vert_c0	= O.c0;
	v2p_vert_fog	= O.fog;
#if SSR_QUALITY > 0
	v2p_vert_pos	=  O.position_w;
#endif
	gl_Position	= O.hpos;
}
 


uniform float4x4 m_texgen;

v2p _main (v_vert v)
{
	v.N		= unpack_D3DCOLOR(v.N);
	v.T		= unpack_D3DCOLOR(v.T);
	v.B		= unpack_D3DCOLOR(v.B);
	v.color		= unpack_D3DCOLOR(v.color);

	v2p		o;

        float4	P	= v.P;                				// world
        float3	NN	= unpack_normal(v.N);
		P	= watermove(P);

	o.v2point	= P.xyz-eye_position;
#if SSR_QUALITY > 0
	o.position_w= P;
#endif
	o.tbase		= unpack_tc_base	(v.uv,v.T.w,v.B.w);	// copy tc
	o.tnorm0	= watermove_tc		(o.tbase*W_DISTORT_BASE_TILE_0, P.xz, W_DISTORT_AMP_0);
	o.tnorm1	= watermove_tc		(o.tbase*W_DISTORT_BASE_TILE_1, P.xz, W_DISTORT_AMP_1);

	// Calculate the 3x3 transform from tangent space to eye-space
	// TangentToEyeSpace = object2eye * tangent2object
	//                     = object2eye * transpose(object2tangent) (since the inverse of a rotation is its transpose)
	float3		N	= unpack_bx2(v.N);			// just scale (assume normal in the -0.5, 0.5)
	float3		T	= unpack_bx2(v.T);			//
	float3		B	= unpack_bx2(v.B);			//
	float3x3	xform	= mul(float3x3(m_W), float3x3(T,B,N));
	// The pixel shader operates on the bump-map in [0..1] range
	// Remap this range in the matrix, anyway we are pixel-shader limited :)
	// ...... [ 2  0  0  0]
	// ...... [ 0  2  0  0]
	// ...... [ 0  0  2  0]
	// ...... [-1 -1 -1  1]
	// issue: strange, but it's slower :(
	// issue: interpolators? dp4? VS limited? black magic?

	// Feed this transform to pixel shader
	o.M1		= xform[0];
	o.M2		= xform[1];
	o.M3		= xform[2];

	float3 L_rgb	= v.color.xyz;						// precalculated RGB lighting
	float3 L_hemi	= v_hemi(N)*v.N.w;					// hemisphere
	float3 L_sun	= v_sun(N)*v.color.w;					// sun
	float3 L_final	= L_rgb + L_hemi + L_sun + L_ambient.rgb;

	o.hpos		= mul		(m_VP, P);				// xform, input in world coords
	o.fog		= saturate	(calc_fogging(v.P));
	//o.fog		*= o.fog;
	o.c0		= float4	(L_final, 1.0);

#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	o.tctexgen	= mul		(m_texgen, P);
	float3	Pe	= mul		(m_V, P).xyz;
	o.tctexgen.z	= Pe.z;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)

	return o;
}
 
 
