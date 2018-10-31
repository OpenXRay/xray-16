
out vec4 SV_Target;

struct v2p
{
	float2  tc0	; // TEXCOORD0;		// base
	float3  tc1	; // TEXCOORD1;		// environment
	float3  c0	; // COLOR0;		// sun
	float   fog	; // FOG;
};

layout(location = TEXCOORD0) 		in float2	v2p_model_tc0		; // TEXCOORD0;
layout(location = TEXCOORD1) 		in float3	v2p_model_tc1		; // TEXCOORD1;
layout(location = COLOR0) 		in float3	v2p_model_c0		; // COLOR0;		
layout(location = FOG) 			in float	v2p_model_fog		; // FOG;

float4 _main ( v2p I );

void main()
{
	v2p		I;
	I.tc0		= v2p_model_tc0;
	I.tc1		= v2p_model_tc1;
	I.c0		= v2p_model_c0;
	I.fog		= v2p_model_fog;

	SV_Target	= _main (I);
}
