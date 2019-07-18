out vec4 SV_Target;
in vec4 gl_FragCoord;

#ifdef MSAA_OPTIMIZATION
in int gl_SampleID;
#endif

struct	_input
{
#ifdef USE_VTF
	float4	tc0	; // TEXCOORD0;	// tc.xy, tc.w = tonemap scale
#else // USE_VTF
	float2	tc0	; // TEXCOORD0;	// tc.xy
#endif // USE_VTF
	float2	tcJ	; // TEXCOORD1;	// jitter coords
	float4	pos2d	; // SV_Position;
};

#ifdef USE_VTF
layout(location = TEXCOORD0)	in float4	p_combine_tc0	; // TEXCOORD0;	// tc.xy, tc.w = tonemap scale
#else // USE_VTF
layout(location = TEXCOORD0)	in float2	p_combine_tc0	; // TEXCOORD0;	// tc.xy
#endif // USE_VTF
layout(location = TEXCOORD1)	in float2	p_combine_tcJ	; // TEXCOORD1;	// jitter coords

#ifndef MSAA_OPTIMIZATION
float4 _main ( _input I );
#else
float4 _main ( _input I, uint iSample );
#endif

void main()
{
	_input		I;
	I.tc0			= p_combine_tc0;
	I.tcJ			= p_combine_tcJ;
	I.pos2d		= gl_FragCoord;

#ifndef MSAA_OPTIMIZATION
	SV_Target	= _main ( I );
#else
	SV_Target	= _main ( I, gl_SampleID );
#endif
}
