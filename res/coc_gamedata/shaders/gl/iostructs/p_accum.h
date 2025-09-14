
out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif
#ifdef MSAA_OPTIMIZATION
in int gl_SampleID;
#endif

layout(location = TEXCOORD0)	in float4 	p_accum_omni_tc		; // TEXCOORD0;

#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
float4 _main ( float4 tc, float4 pos2d, uint iSample );
#else
float4 _main ( float4 tc, uint iSample );
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
float4 _main ( float4 tc, float4 pos2d );
#else
float4 _main ( float4 tc );
#endif
#endif

void main()
{
#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
	SV_Target = _main ( p_accum_omni_tc, gl_FragCoord, gl_SampleID );
#else
	SV_Target = _main ( p_accum_omni_tc, gl_SampleID );
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
	SV_Target = _main ( p_accum_omni_tc, gl_FragCoord );
#else
	SV_Target = _main ( p_accum_omni_tc );
#endif
#endif
}
