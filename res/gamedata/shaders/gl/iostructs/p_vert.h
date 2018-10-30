
out vec4 SV_Target;

struct v2p
{
	float2 Tex0	; // TEXCOORD0;
	float3 c0	; // COLOR0;		// c0=all lighting
	float  fog	; // FOG;
};

layout(location = TEXCOORD0) 		in float2	v2p_vert_tc		; // TEXCOORD0;
layout(location = COLOR0) 		in float3	v2p_vert_c		; // COLOR0;		// c0=all lighting
layout(location = FOG) 			in float	v2p_vert_fog		; // FOG;

float4 _main ( v2p I );

void main()
{
	v2p		I;
	I.Tex0		= v2p_vert_tc;
	I.c0	 	= v2p_vert_c;
	I.fog		= v2p_vert_fog;

	SV_Target	= _main (I);
}
