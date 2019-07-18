
out vec4 SV_Target0;
out vec4 SV_Target1;

struct	_input      
{
#ifdef USE_VTF
	float4	tc0		; // TEXCOORD0;	// tc.xy, tc.w = tonemap scale
#else // USE_VTF
	float2	tc0		; // TEXCOORD0;	// tc.xy
#endif // USE_VTF
};

struct	_out
{
	float4	low		; // SV_Target0;
	float4	high		; // SV_Target1;
};

#ifdef USE_VTF
layout(location = TEXCOORD0)	in float4	p_volumetric_tc0;	// TEXCOORD0;	// tc.xy, tc.w = tonemap scale
#else // USE_VTF
layout(location = TEXCOORD0)	in float2	p_volumetric_tc0;	// TEXCOORD0;	// tc.xy
#endif // USE_VTF

_out _main( _input I );

void main()
{
	_input		I;
	I.tc0		= p_volumetric_tc0;

	_out O		= _main ( I );

	SV_Target0	= O.low;
	SV_Target1	= O.high;
}
