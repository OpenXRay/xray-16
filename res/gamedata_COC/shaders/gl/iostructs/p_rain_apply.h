
out vec4 SV_Target;
#ifdef MSAA_OPTIMIZATION
in int gl_SampleID;
#endif

layout(location = TEXCOORD0)	in float2	p_rain_tc	; // TEXCOORD0;
layout(location = TEXCOORD1)	in float2	p_rain_tcJ	; // TEXCOORD1; 

#ifdef MSAA_OPTIMIZATION
float4 _main ( float2 tc, float2 tcJ, uint iSample );
#else
float4 _main ( float2 tc, float2 tcJ );
#endif

void main()
{
#ifdef MSAA_OPTIMIZATION
	SV_Target	= _main ( p_rain_tc, p_rain_tcJ, gl_SampleID );
#else
	SV_Target	= _main ( p_rain_tc, p_rain_tcJ );
#endif
}
