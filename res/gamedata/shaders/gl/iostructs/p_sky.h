
out vec4 SV_Target0;
out vec4 SV_Target1;

struct         v2p
{
	float4	factor	; // COLOR0;        // for SM3 - factor.rgb - tonemap-prescaled
	float3	tc0		; // TEXCOORD0;
	float3	tc1		; // TEXCOORD1;
};
struct        _out
{
	float4	low		; // SV_Target0;
	float4	high	; // SV_Target1;
};

layout(location = COLOR0)		in float4	p_sky_factor; // COLOR0;        // for SM3 - factor.rgb - tonemap-prescaled
layout(location = TEXCOORD0)	in float3	p_sky_tc0	; // TEXCOORD0;
layout(location = TEXCOORD1)	in float3	p_sky_tc1	; // TEXCOORD1;

_out _main( v2p I );

void main()
{
	v2p			I;
	I.factor	= p_sky_factor;
	I.tc0 		= p_sky_tc0;
	I.tc1 		= p_sky_tc1;

	_out O		= _main (I);

	SV_Target0	= O.low;
	SV_Target1	= O.high;
}
