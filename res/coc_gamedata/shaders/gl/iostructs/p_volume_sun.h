
out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif // GBUFFER_OPTIMIZATION
#ifdef MSAA_OPTIMIZATION
in int gl_SampleID;
#endif // MSAA_OPTIMIZATION

layout(location = TEXCOORD0)	in 	float4 	p_volume_tc; // TEXCOORD0;
#ifdef USE_SJITTER
layout(location = TEXCOORD1)	in 	float4 	p_volume_tcJ; // TEXCOORD1;
#endif // USE_SJITTER

#ifdef MSAA_OPTIMIZATION
float4 _main ( v2p_volume I, uint iSample );
#else
float4 _main ( v2p_volume I );
#endif

void main()
{
	v2p_volume	I;
	I.tc		= p_volume_tc;
#ifdef USE_SJITTER
	I.tcJ 		= p_volume_tcJ;
#endif // USE_SJITTER
#ifdef GBUFFER_OPTIMIZATION
	I.hpos	= gl_FragCoord;
#endif // GBUFFER_OPTIMIZATION

#ifdef MSAA_OPTIMIZATION
	SV_Target	= _main ( I, gl_SampleID );
#else
	SV_Target	= _main ( I );
#endif
}
