
out gl_PerVertex { vec4 gl_Position; };

struct v_static_color
{
	float4	Nh	; // NORMAL;	// (nx,ny,nz,hemi occlusion)
	float4	T	; // TANGENT;	// tangent
	float4	B	; // BINORMAL;	// binormal
	float2	tc	; // TEXCOORD0;	// (u,v)
#ifdef	USE_LM_HEMI
	float2	lmh	; // TEXCOORD1;	// (lmu,lmv)
#endif
	float4	color	; // COLOR0;	// (r,g,b,dir-occlusion)	//	Swizzle before use!!!
	float4	P	; // POSITION;	// (float,float,float,1)
};

struct v2p
{
	float4 hpos	; // SV_Position;
	float2 Tex0	; // TEXCOORD0;
	float3 c0	; // COLOR0;		// c0=all lighting
	float  fog	; // FOG;
};

layout(location = POSITION)		in float4	v_static_color_P	; // POSITION;		// (float,float,float,1)
layout(location = NORMAL)		in float4	v_static_color_N	; // NORMAL;		// (nx,ny,nz,hemi occlusion)
layout(location = TANGENT)		in float4	v_static_color_T	; // TANGENT;		// tangent
layout(location = BINORMAL)		in float4	v_static_color_B	; // BINORMAL;		// binormal
layout(location = TEXCOORD0)		in float2	v_static_color_tc	; // TEXCOORD0;		// (u,v)
#ifdef USE_LM_HEMI
layout(location = TEXCOORD1)		in float2	v_static_color_lmh	; // TEXCOORD1;		// (lmu,lmv)
#endif
layout(location = COLOR0)		in float4	v_static_color_c	; // COLOR0;		// (r,g,b,dir-occlusion)


layout(location = TEXCOORD0) 		out float2	v2p_vert_tc		; // TEXCOORD0;
layout(location = COLOR0) 		out float3	v2p_vert_c		; // COLOR0;		// c0=all lighting
layout(location = FOG) 			out float	v2p_vert_fog		; // FOG;

v2p _main ( v_static_color I );

void main()
{
	v_static_color	I;
	I.P		= v_static_color_P;
	I.Nh		= v_static_color_N;
	I.T		= v_static_color_T;
	I.B		= v_static_color_B;
	I.tc		= v_static_color_tc;
#ifdef USE_LM_HEMI
	I.lmh		= v_static_color_lmh;
#endif
	I.color		= v_static_color_c;

	v2p O 		= _main (I);

	v2p_vert_tc	= O.Tex0;
	v2p_vert_c	= O.c0;
	v2p_vert_fog	= O.fog;
	gl_Position	= O.hpos;
}
