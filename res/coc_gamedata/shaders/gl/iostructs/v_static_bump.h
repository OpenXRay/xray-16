
out gl_PerVertex { vec4 gl_Position; };

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
#define	v_in	v_static_color	
#else
#define	v_in	v_static
#endif

layout(location = NORMAL)		in float4	v_static_Nh		; // NORMAL;	// (nx,ny,nz,hemi occlusion)
layout(location = TANGENT)		in float4	v_static_T		; // TANGENT;	// tangent
layout(location = BINORMAL)		in float4	v_static_B		; // BINORMAL;	// binormal
layout(location = TEXCOORD0)	in float2	v_static_tc		; // TEXCOORD0;	// (u,v)
#ifdef	USE_LM_HEMI
layout(location = TEXCOORD1)	in float2	v_static_lmh	; // TEXCOORD1;	// (lmu,lmv)
#endif
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
layout(location = COLOR0)		in float4	v_static_C	; // COLOR0;	// (r,g,b,dir-occlusion)	//	Swizzle before use!!!
#endif
layout(location = POSITION)		in float4	v_static_P		; // POSITION;	// (float,float,float,1)

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
layout(location = TEXCOORD0) 	out float4	v2p_bumped_tcdh	; // TEXCOORD0;	// Texture coordinates,         w=sun_occlusion
#else
layout(location = TEXCOORD0) 	out float2	v2p_bumped_tcdh	; // TEXCOORD0;	// Texture coordinates
#endif
layout(location = TEXCOORD1) 	out float4	v2p_bumped_position; // TEXCOORD1;	// position + hemi
layout(location = TEXCOORD2) 	out float3	v2p_bumped_M1	; // TEXCOORD2;	// nmap 2 eye - 1
layout(location = TEXCOORD3) 	out float3	v2p_bumped_M2	; // TEXCOORD3;	// nmap 2 eye - 2
layout(location = TEXCOORD4) 	out float3	v2p_bumped_M3	; // TEXCOORD4;	// nmap 2 eye - 3
#ifdef USE_TDETAIL
layout(location = TEXCOORD5) 	out float2	v2p_bumped_tcdbump; // TEXCOORD5;	// d-bump
#endif
#ifdef USE_LM_HEMI
layout(location = TEXCOORD6) 	out float2	v2p_bumped_lmh	; // TEXCOORD6;	// lm-hemi
#endif

v2p_bumped _main( v_in I );

void main()
{
	v_in		I;
	I.Nh		= v_static_Nh;
	I.T			= v_static_T;
	I.B			= v_static_B;
	I.tc		= v_static_tc;
#ifdef	USE_LM_HEMI
	I.lmh		= v_static_lmh;
#endif
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	I.color		= v_static_C;
#endif
	I.P			= v_static_P;

	v2p_bumped O = _main (I);

	v2p_bumped_tcdh	= O.tcdh;
	v2p_bumped_position = O.position;
	v2p_bumped_M1 = O.M1;
	v2p_bumped_M2 = O.M2;
	v2p_bumped_M3 = O.M3;
#ifdef USE_TDETAIL
	v2p_bumped_tcdbump = O.tcdbump;
#endif
#ifdef	USE_LM_HEMI
	v2p_bumped_lmh = O.lmh;
#endif
	gl_Position = O.hpos;
}
