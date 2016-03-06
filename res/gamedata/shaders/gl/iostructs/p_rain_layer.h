
out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif

layout(location = TEXCOORD0)	in float2	p_rain_tc	; // TEXCOORD0;
layout(location = TEXCOORD1)	in float2	p_rain_tcJ	; // TEXCOORD1; 

#ifdef GBUFFER_OPTIMIZATION
float4 _main ( float2 tc, float2 tcJ, float4 pos2d );
#else
float4 _main ( float2 tc, float2 tcJ );
#endif

void main()
{
#ifdef GBUFFER_OPTIMIZATION
	SV_Target	= _main ( p_rain_tc, p_rain_tcJ, gl_FragCoord );
#else
	SV_Target	= _main ( p_rain_tc, p_rain_tcJ );
#endif
}
