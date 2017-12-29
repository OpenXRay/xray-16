
out vec4 SV_Target0;
out vec4 SV_Target1;
#ifndef GBUFFER_OPTIMIZATION
out vec4 SV_Target2;
#endif
#ifdef EXTEND_F_DEFFER
out int gl_SampleMask[];
#endif
#ifdef	MSAA_ALPHATEST_DX10_1_ATOC
in vec4 gl_FragCoord;
#endif

struct 	p_particle
{
	float4 	color	; // COLOR0;
	p_flat	base;	
};

layout(location = COLOR0)		in float4 	p_particle_color; // COLOR0;
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

#ifdef	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer 	_main	( p_particle II, float4 pos2d );
#else	//	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer 	_main	( p_particle II );
#endif	//	MSAA_ALPHATEST_DX10_1_ATOC

void main()
{
	p_particle		I;
	I.color			= p_particle_color;
	I.base.tcdh		= p_flat_tcdh;
	I.base.position = p_flat_position;
	I.base.N		= p_flat_N;
#ifdef USE_TDETAIL
	I.base.tcdbump 	= p_flat_tcdbump;
#endif
#ifdef USE_LM_HEMI
	I.base.lmh		= p_flat_lmh;
#endif

#ifdef	MSAA_ALPHATEST_DX10_1_ATOC
	f_deffer O	= _main	( I, gl_FragCoord );
#else	//	MSAA_ALPHATEST_DX10_1_ATOC
	f_deffer O	= _main	( I );
#endif	//	MSAA_ALPHATEST_DX10_1_ATOC

	SV_Target0 = O.position;
#ifdef GBUFFER_OPTIMIZATION
	SV_Target1 = O.C;
#else
	SV_Target1 = O.Ne;
	SV_Target2 = O.C;
#endif
#ifdef EXTEND_F_DEFFER
	gl_SampleMask[0] = O.mask;
#endif
}
