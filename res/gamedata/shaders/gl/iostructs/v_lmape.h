
out gl_PerVertex { vec4 gl_Position; };

struct v_static
{
	float4	Nh	; // NORMAL;	// (nx,ny,nz,hemi occlusion)
	float4	T	; // TANGENT;	// tangent
	float4	B	; // BINORMAL;	// binormal
	float2	tc	; // TEXCOORD0;	// (u,v)
#ifdef	USE_LM_HEMI
	float2	lmh	; // TEXCOORD1;	// (lmu,lmv)
#endif
	float4	P	; // POSITION;	// (float,float,float,1)
};

struct v2p
{
	float4  hpos	;	// SV_Position;
 	float2 	tc0	; 	// TEXCOORD0;		// base
 	float2 	tc1	; 	// TEXCOORD1;		// lmap
	float2	tc2	; 	// TEXCOORD2;		// hemi
	float3	tc3	; 	// TEXCOORD3;		// env
  	float3	c0	; 	// COLOR0;
	float3	c1	; 	// COLOR1;
	float   fog	; 	// FOG;
};

layout(location = POSITION)		in float4	v_static_P		; // POSITION;		// (float,float,float,1)
layout(location = NORMAL)		in float4	v_static_N		; // NORMAL;		// (nx,ny,nz,hemi occlusion)
layout(location = TANGENT)		in float4	v_static_T		; // TANGENT;		// tangent
layout(location = BINORMAL)		in float4	v_static_B		; // BINORMAL;		// binormal
layout(location = TEXCOORD0)		in float2	v_static_tc		; // TEXCOORD0;		// (u,v)
#ifdef USE_LM_HEMI
layout(location = TEXCOORD1)		in float2	v_static_lmh		; // TEXCOORD1;		// (lmu,lmv)
#endif


layout(location = TEXCOORD0) 		out float2	v2p_lmape_tc0		; // TEXCOORD0;		// base
layout(location = TEXCOORD1) 		out float2	v2p_lmape_tc1		; // TEXCOORD1;		// lmap
layout(location = TEXCOORD2) 		out float2	v2p_lmape_tc2		; // TEXCOORD2;		// hemi
layout(location = TEXCOORD3) 		out float3	v2p_lmape_tc3		; // TEXCOORD3;		// env
layout(location = COLOR0) 		out float3	v2p_lmape_c0		; // COLOR0;
layout(location = COLOR1) 		out float3	v2p_lmape_c1		; // COLOR1;
layout(location = FOG) 			out float	v2p_lmape_fog		; // FOG;

v2p _main ( v_static I );

void main()
{
	v_static	I;
	I.P		= v_static_P;
	I.Nh		= v_static_N;
	I.T		= v_static_T;
	I.B		= v_static_B;
	I.tc		= v_static_tc;
#ifdef USE_LM_HEMI
	I.lmh		= v_static_lmh;
#endif

	v2p O 		= _main (I);

	v2p_lmape_tc0	= O.tc0;
	v2p_lmape_tc1	= O.tc1;
	v2p_lmape_tc2	= O.tc2;
	v2p_lmape_tc3	= O.tc3;
	v2p_lmape_c0	= O.c0;
	v2p_lmape_c1	= O.c1;
	v2p_lmape_fog	= O.fog;
	gl_Position	= O.hpos;
}
