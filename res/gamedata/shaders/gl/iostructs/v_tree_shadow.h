
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITION)		in float4	v_tree_P		; // POSITION;		// (float,float,float,1)
layout(location = NORMAL)		in float4	v_tree_Nh		; // NORMAL;		// (nx,ny,nz)
layout(location = TANGENT)		in float3	v_tree_T		; // TANGENT;		// tangent
layout(location = BINORMAL)		in float3	v_tree_B		; // BINORMAL;		// binormal
layout(location = TEXCOORD0)	in int4		v_tree_tc		; // TEXCOORD0;	// (u,v,frac,???)

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
layout(location = TEXCOORD0) 	out float4	v2p_flat_tcdh	; // TEXCOORD0;	// Texture coordinates,         w=sun_occlusion
#else
layout(location = TEXCOORD0) 	out float2	v2p_flat_tcdh	; // TEXCOORD0;	// Texture coordinates
#endif
layout(location = TEXCOORD1) 	out float4	v2p_flat_position; // TEXCOORD1;	// position + hemi
layout(location = TEXCOORD2) 	out float3	v2p_flat_N		; // TEXCOORD2;	// Eye-space normal        (for lighting)
#ifdef USE_TDETAIL
layout(location = TEXCOORD3) 	out float2	v2p_flat_tcdbump; // TEXCOORD3;	// d-bump
#endif
#ifdef USE_LM_HEMI
layout(location = TEXCOORD4) 	out float2	v2p_flat_lmh	; // TEXCOORD4;	// lm-hemi
#endif

#ifdef	USE_AREF
v2p_shadow_direct_aref _main ( v_shadow_direct_aref I );
#else	//	USE_AREF
v2p_shadow_direct _main ( v_shadow_direct I );
#endif	//	USE_AREF

void main()
{
	v_tree		I;
	I.P			= v_tree_P;
	I.Nh		= v_tree_Nh;
	I.T			= v_tree_T;
	I.B			= v_tree_B;
	I.tc		= v_tree_tc;

#ifdef	USE_AREF
	v2p_shadow_direct_aref O = _main (I);
	v2p_shadow_tc0 = O.tc0;
#else	//	USE_AREF
	v2p_shadow_direct O = _main (I);
#endif	//	USE_AREF

	gl_Position = O.hpos;
}
