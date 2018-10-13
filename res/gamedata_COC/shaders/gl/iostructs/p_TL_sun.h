
out vec4 SV_Target;
#ifdef MSAA_OPTIMIZATION
in int gl_SampleID;
#endif
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif

layout(location = TEXCOORD0)	in float2	p_TL_Tex0	; // TEXCOORD0;
layout(location = COLOR)		in float4	p_TL_Color	; // COLOR; 

#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
float4 _main ( p_TL I, float4 pos2d, uint iSample );
#else
float4 _main ( p_TL I, uint iSample );
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
float4 _main ( p_TL I, float4 pos2d );
#else
float4 _main ( p_TL I );
#endif
#endif

void main()
{
	p_TL		I;
	I.Tex0		= p_TL_Tex0;
	I.Color 	= p_TL_Color;

#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
	SV_Target	= _main ( I, gl_FragCoord, gl_SampleID );
#else
	SV_Target	= _main ( I, gl_SampleID );
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
	SV_Target	= _main ( I, gl_FragCoord );
#else
	SV_Target	= _main ( I );
#endif
#endif
}
