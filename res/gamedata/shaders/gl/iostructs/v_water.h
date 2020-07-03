
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
