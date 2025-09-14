
out vec4 SV_Target;

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
layout(location = TEXCOORD0)	in float4	p_flat_tcdh		; // TEXCOORD0;	// Texture coordinates,         w=sun_occlusion
#else
layout(location = TEXCOORD0)	in float2	p_flat_tcdh		; // TEXCOORD0;	// Texture coordinates
#endif
layout(location = TEXCOORD1)	in float4	p_flat_position; // TEXCOORD1;	// position + hemi
layout(location = TEXCOORD2)	in float3	p_flat_N		; // TEXCOORD2;	// Eye-space normal        (for lighting)
#ifdef USE_TDETAIL
layout(location = TEXCOORD3)	in float2	p_flat_tcdbump	; // TEXCOORD3;	// d-bump
#endif
#ifdef USE_LM_HEMI
layout(location = TEXCOORD4)	in float2	p_flat_lmh		; // TEXCOORD4;	// lm-hemi
#endif

float4 	_main	( p_bumped I );

void main()
{
	p_flat		I;
	I.tcdh		= p_flat_tcdh;
	I.position 	= p_flat_position;
	I.N			= p_flat_N;
#ifdef USE_TDETAIL
	I.tcdbump 	= p_flat_tcdbump;
#endif
#ifdef USE_LM_HEMI
	I.lmh		= p_flat_lmh;
#endif

	SV_Target	= _main	( I );
}
