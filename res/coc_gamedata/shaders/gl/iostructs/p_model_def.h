
out vec4 SV_Target;

struct 	v2p
{
 	float2 	tc0; // 		TEXCOORD0;	// base
// 	float2 	tc1; // 		TEXCOORD1;	// lmap
  	float3	c0; //			COLOR0;		// sun
};

layout(location = TEXCOORD0)	in float2	p_model_tc0	; // TEXCOORD0;	// base
//layout(location = TEXCOORD1)	in float2	p_model_tc1	; // TEXCOORD1;	// lmap
layout(location = COLOR0)		in float3	p_model_c0	; // COLOR0; 	// sun

float4 _main ( v2p I );

void main()
{
	v2p			I;
	I.tc0		= p_model_tc0;
//	I.tc1		= p_model_tc1;
	I.c0	 	= p_model_c0;

	SV_Target	= _main (I);
}
