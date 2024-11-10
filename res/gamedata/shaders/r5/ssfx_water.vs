#include "common.h"

uniform float4 shader_param_7;

struct v_vert
{
	float4	P		: POSITION;		// (float,float,float,1)
	float4	N		: NORMAL;		// (nx,ny,nz,hemi occlusion)
	float4	T		: TANGENT;
	float4	B		: BINORMAL;
	float4	color	: COLOR0;		// (r,g,b,dir-occlusion)
	int2	uv		: TEXCOORD0;	// (u0,v0)
};

struct   vf
{
	float2	tbase		: TEXCOORD0;	// base
	float4	tnorm0		: TEXCOORD1;	// nm0
	float3	position_w	: TEXCOORD2;	// nm1
	float3	M1			: TEXCOORD3;
	float3	M2			: TEXCOORD4;
	float3	M3			: TEXCOORD5;
	float3	v2point_w	: TEXCOORD6;
	float4	tctexgen	: TEXCOORD7;
	float4	c0			: COLOR0;
	float	fog			: FOG;
	float4	hpos		: SV_Position;
};

uniform float4x4 m_texgen;

vf main (v_vert v)
{
	v.N		=	unpack_D3DCOLOR(v.N);
	v.T		=	unpack_D3DCOLOR(v.T);
	v.B		=	unpack_D3DCOLOR(v.B);
	v.color	=	unpack_D3DCOLOR(v.color);

	vf o;

	float4 P = v.P;
	o.position_w = P.xyz;
	
	//P = watermove(P);

	o.v2point_w = P - eye_position;
	o.tbase = unpack_tc_base(v.uv * 1.5f, v.T.w, v.B.w);

	// Calculate the 3x3 transform from tangent space to eye-space
	// TangentToEyeSpace = object2eye * tangent2object
	// = object2eye * transpose(object2tangent) (since the inverse of a rotation is its transpose)
	float3 N = unpack_bx2(v.N);
	float3 T = unpack_bx2(v.T);
	float3 B = unpack_bx2(v.B);
	float3x3 xform = mul((float3x3)m_W, float3x3(
							T.x,B.x,N.x,
							T.y,B.y,N.y,
							T.z,B.z,N.z
						));
	// The pixel shader operates on the bump-map in [0..1] range
	// Remap this range in the matrix, anyway we are pixel-shader limited :)
	// ...... [ 2  0  0  0]
	// ...... [ 0  2  0  0]
	// ...... [ 0  0  2  0]
	// ...... [-1 -1 -1  1]
	// issue: strange, but it's slower :(
	// issue: interpolators? dp4? VS limited? black magic?

	// Feed this transform to pixel shader
	o.M1 = xform[0];
	o.M2 = xform[1];
	o.M3 = xform[2];

	float3 L_rgb	= v.color.xyz;			// precalculated RGB lighting
	float3 L_hemi	= v_hemi(N) * v.N.w;		// hemisphere
	float3 L_sun	= v_sun(N) * v.color.w;	// sun
	float3 L_final	= L_rgb + L_hemi + L_sun + L_ambient;

	o.hpos	= mul(m_VP, P);
	o.fog	= saturate(calc_fogging(v.P));
	o.c0	= float4(L_final,1);

	o.tctexgen = mul( m_texgen, P);
	float3 Pe = mul(m_V,  P);
	o.tctexgen.z = Pe.z;
	return o;
}