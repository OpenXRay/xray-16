
out vec4 SV_Target;

struct 	v2p
{
	float2	tc0	; // TEXCOORD0;	// base
};

layout(location = TEXCOORD0)	in float2	p_simple_tc0	; // TEXCOORD0;	// base

float4 _main ( v2p I );

void main()
{
	v2p			I;
	I.tc0		= p_simple_tc0;

	SV_Target	= _main ( I );
}
