
out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif
#ifdef MSAA_OPTIMIZATION
in int gl_SampleID;
#endif

layout(location = TEXCOORD0)	in 	float4 	p_volume_tc		; // TEXCOORD0;
#ifdef 	USE_SJITTER
layout(location = TEXCOORD1)	in 	float4 	p_volume_tcJ	; // TEXCOORD1;
#endif

#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
float4 _main( p_volume I, float4 pos2d, uint iSample );
#else
float4 _main( p_volume I, uint iSample  );
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
float4 _main( p_volume I, float4 pos2d );
#else
float4 _main( p_volume I );
#endif
#endif

void main()
{
	p_volume	I;
	I.tc		= p_volume_tc;
#ifdef 	USE_SJITTER
	I.tcJ 		= p_volume_tcJ;
#endif

#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
	SV_Target	= _main( I, gl_FragCoord, gl_SampleID );
#else
	SV_Target	= _main( I, gl_SampleID  );
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
	SV_Target	= _main( I, gl_FragCoord );
#else
	SV_Target	= _main( I );
#endif
#endif
}
