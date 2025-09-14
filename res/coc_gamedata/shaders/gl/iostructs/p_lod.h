
out vec4 SV_Target0;
#ifndef	ATOC
out vec4 SV_Target1;
#ifndef GBUFFER_OPTIMIZATION
out vec4 SV_Target2;
#endif // GBUFFER_OPTIMIZATION
#ifdef EXTEND_F_DEFFER
out int gl_SampleMask[];
#endif // EXTEND_F_DEFFER
#ifdef	MSAA_ALPHATEST_DX10_1_ATOC
in vec4 gl_FragCoord;
#endif // MSAA_ALPHATEST_DX10_1_ATOC
#endif // #endif 

struct v2p
{
	float3	Pe	; 	// TEXCOORD0;
 	float2 	tc0	; 	// TEXCOORD1;		// base0
 	float2 	tc1	; 	// TEXCOORD2;		// base1
	float4 	af	; 	// COLOR1;		// alpha&factor //skyloader: COLOR1? maybe COLOR0?
};

layout(location = TEXCOORD0) 		in float3	v2p_lod_Pe		; // TEXCOORD0;		// base
layout(location = TEXCOORD1) 		in float2	v2p_lod_tc0		; // TEXCOORD1;		// lmap
layout(location = TEXCOORD2) 		in float2	v2p_lod_tc1		; // TEXCOORD2;		// hemi
layout(location = COLOR1) 		in float4	v2p_lod_af		; // COLOR1;

#ifdef	ATOC
float4 _main ( v2p I );
#else	// ATOC
#ifdef	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer _main ( v2p I, float4 pos2d );
#else	//	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer _main ( v2p I );
#endif	//	MSAA_ALPHATEST_DX10_1_ATOC
#endif	// ATOC

void main()
{
	v2p		I;
	I.Pe		= v2p_lod_Pe;
	I.tc0		= v2p_lod_tc0;
	I.tc1		= v2p_lod_tc1;
	I.af		= v2p_lod_af;

#ifdef	ATOC
	SV_Target	= _main (I);
#else	// ATOC
#ifdef	MSAA_ALPHATEST_DX10_1_ATOC
	f_deffer O	= _main (I, gl_FragCoord);
#else
	f_deffer O	= _main (I);
#endif	// MSAA_ALPHATEST_DX10_1_ATOC

#endif	// ATOC

	SV_Target0 = O.position;
#ifdef GBUFFER_OPTIMIZATION
	SV_Target1 = O.C;
#else
	SV_Target1 = O.Ne;
	SV_Target2 = O.C;
#endif	// GBUFFER_OPTIMIZATION
#ifdef EXTEND_F_DEFFER
	gl_SampleMask[0] = O.mask;
#endif
}
