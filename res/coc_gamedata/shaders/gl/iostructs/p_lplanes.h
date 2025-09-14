
out vec4 SV_Target;

struct v2p
{
 	float2 	tc0	; // TEXCOORD0;		// base
  	float4	c0	; // COLOR0;		// sun
};

layout(location = TEXCOORD0) 		in float2	v2p_lplanes_tc0		; // TEXCOORD0;		// base
layout(location = COLOR0) 		in float4	v2p_lplanes_c0		; // COLOR0;		// sun

float4 _main ( v2p I );

void main()
{
	v2p		I;
	I.tc0		= v2p_lplanes_tc0;
	I.c0	 	= v2p_lplanes_c0;

	SV_Target	= _main (I);
}
