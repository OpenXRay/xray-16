
out gl_PerVertex { vec4 gl_Position; };

struct v_vert
{
	float4 	P	;	// POSITION;		// (float,float,float,1)
	float4	N	;	// NORMAL;		// (nx,ny,nz,hemi occlusion)
	float4 	T	;	// TANGENT;
	float4 	B	;	// BINORMAL;
	float4	color	;	// COLOR0;		// (r,g,b,dir-occlusion)
	int2 	uv	;	// TEXCOORD0;		// (u0,v0)
};
struct v2p
{
	float4  hpos	;	// SV_Position;
	float2  tbase	;	// TEXCOORD0;
	float2  tdist0	;	// TEXCOORD1;
	float2  tdist1	;	// TEXCOORD2;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	float4  tctexgen;	// TEXCOORD3;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
};

layout(location = POSITION)		in float4	v_vert_P		; // POSITION;		// (float,float,float,1)
layout(location = NORMAL)		in float4	v_vert_N		; // NORMAL;		// (nx,ny,nz,hemi occlusion)
layout(location = TANGENT)		in float4	v_vert_T		; // TANGENT;		// tangent
layout(location = BINORMAL)		in float4	v_vert_B		; // BINORMAL;		// binormal
layout(location = COLOR0)		in float4	v_vert_color		; // COLOR0;		// (r,g,b,dir-occlusion)
layout(location = TEXCOORD0)		in int2		v_vert_uv		; // TEXCOORD0;		// (u0,v0)

layout(location = TEXCOORD0) 		out float2	v2p_vert_tbase		; // TEXCOORD0;
layout(location = TEXCOORD1) 		out float2	v2p_vert_tdist0		; // TEXCOORD1;
layout(location = TEXCOORD2) 		out float2	v2p_vert_tdist1		; // TEXCOORD2;		
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
layout(location = TEXCOORD3) 		out float4	v2p_vert_tctexgen	; // TEXCOORD3;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)

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
	v2p_vert_tdist0	= O.tdist0;
	v2p_vert_tdist1	= O.tdist1;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	v2p_vert_tctexgen = O.tctexgen;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	gl_Position	= O.hpos;
}
