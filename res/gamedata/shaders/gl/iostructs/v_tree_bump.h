
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITION)		in float4	v_tree_P		; // POSITION;		// (float,float,float,1)
layout(location = NORMAL)		in float4	v_tree_Nh		; // NORMAL;		// (nx,ny,nz)
layout(location = TANGENT)		in float3	v_tree_T		; // TANGENT;		// tangent
layout(location = BINORMAL)		in float3	v_tree_B		; // BINORMAL;		// binormal
layout(location = TEXCOORD0)	in float4	v_tree_tc		; // TEXCOORD0;	// (u,v,frac,???)

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

v2p_bumped 	_main 	(v_tree I);

void main()
{
	v_tree		I;
	I.P			= v_tree_P;
	I.Nh		= v_tree_Nh;
	I.T			= v_tree_T;
	I.B			= v_tree_B;
	I.tc		= v_tree_tc;

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
