
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITION)		in float4	v_detail_pos	; // POSITION;                // (float,float,float,1)
layout(location = TEXCOORD0)	in float4	v_detail_misc	; // TEXCOORD0;        // (u(Q),v(Q),frac,matrix-id)

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

v2p_flat 	_main (v_detail v);

void main()
{
	v_detail	I;
	I.pos		= v_detail_pos;
	I.misc		= v_detail_misc;

	v2p_flat O	= _main (I);

	v2p_flat_tcdh = O.tcdh;
	v2p_flat_position = O.position;
	v2p_flat_N	= O.N;
#ifdef USE_TDETAIL
	v2p_flat_tcdbump = O.tcdbump;
#endif
#ifdef USE_LM_HEMI
	v2p_flat_lmh = O.lmh;
#endif
	gl_Position = O.hpos;
}
