
out vec4 SV_Target0;
out vec4 SV_Target1;
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

struct	_out
{
	float4	low	; // SV_Target0;
	float4	high	; // SV_Target1;
};

#ifdef USE_VTF
layout(location = TEXCOORD0)	in float4	p_combine_tc0	; // TEXCOORD0;	// tc.xy, tc.w = tonemap scale
#else // USE_VTF
layout(location = TEXCOORD0)	in float2	p_combine_tc0	; // TEXCOORD0;	// tc.xy
#endif // USE_VTF
layout(location = TEXCOORD1)	in float2	p_combine_tcJ	; // TEXCOORD1;	// jitter coords

#ifndef MSAA_OPTIMIZATION
_out _main ( _input I );
#else
_out _main ( _input I, uint iSample );
#endif

void main()
{
	_input		I;
	I.tc0 		= p_combine_tc0;
	I.tcJ 		= p_combine_tcJ;
	I.pos2d		= gl_FragCoord;

#ifndef MSAA_OPTIMIZATION
	_out O		= _main ( I );
#else
	_out O		= _main ( I, gl_SampleID );
#endif

	SV_Target0	= O.low;
	SV_Target1	= O.high;
}
