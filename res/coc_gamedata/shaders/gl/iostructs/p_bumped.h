
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

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
layout(location = TEXCOORD0)	in float4	p_bumped_tcdh	; // TEXCOORD0;	// Texture coordinates,         w=sun_occlusion
#else
layout(location = TEXCOORD0)	in float2	p_bumped_tcdh	; // TEXCOORD0;	// Texture coordinates
#endif
layout(location = TEXCOORD1)	in float4	p_bumped_position; // TEXCOORD1;	// position + hemi
layout(location = TEXCOORD2)	in float3	p_bumped_M1		; // TEXCOORD2;	// nmap 2 eye - 1
layout(location = TEXCOORD3)	in float3	p_bumped_M2		; // TEXCOORD3;	// nmap 2 eye - 2
layout(location = TEXCOORD4)	in float3	p_bumped_M3		; // TEXCOORD4;	// nmap 2 eye - 3
#ifdef USE_TDETAIL
layout(location = TEXCOORD5)	in float2	p_bumped_tcdbump; // TEXCOORD5;	// d-bump
#endif
#ifdef USE_LM_HEMI
layout(location = TEXCOORD6)	in float2	p_bumped_lmh	; // TEXCOORD6;	// lm-hemi
#endif

#ifdef	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer 	_main	( p_bumped I, float4 pos2d );
#else	//	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer 	_main	( p_bumped I );
#endif	//	MSAA_ALPHATEST_DX10_1_ATOC

void main()
{
	p_bumped	I;
	I.tcdh		= p_bumped_tcdh;
	I.position 	= p_bumped_position;
	I.M1		= p_bumped_M1;
	I.M2	 	= p_bumped_M2;
	I.M3		= p_bumped_M3;
#ifdef USE_TDETAIL
	I.tcdbump 	= p_bumped_tcdbump;
#endif
#ifdef USE_LM_HEMI
	I.lmh		= p_bumped_lmh;
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
