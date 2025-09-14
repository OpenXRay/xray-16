
out vec4 SV_Target;

struct v2p
{
 	float2 	tc0	; 	// TEXCOORD0;		// base
 	float2 	tc1	; 	// TEXCOORD1;		// lmap
	float2	tc2	; 	// TEXCOORD2;		// hemi
	float3	tc3	; 	// TEXCOORD3;		// env
  	float3	c0	; 	// COLOR0;
	float3	c1	; 	// COLOR1;
	float   fog	; 	// FOG;
};

layout(location = TEXCOORD0) 		in float2	v2p_lmape_tc0		; // TEXCOORD0;		// base
layout(location = TEXCOORD1) 		in float2	v2p_lmape_tc1		; // TEXCOORD1;		// lmap
layout(location = TEXCOORD2) 		in float2	v2p_lmape_tc2		; // TEXCOORD2;		// hemi
layout(location = TEXCOORD3) 		in float3	v2p_lmape_tc3		; // TEXCOORD3;		// env
layout(location = COLOR0) 		in float3	v2p_lmape_c0		; // COLOR0;
layout(location = COLOR1) 		in float3	v2p_lmape_c1		; // COLOR1;
layout(location = FOG) 			in float	v2p_lmape_fog		; // FOG;

float4 _main ( v2p I );

void main()
{
	v2p		I;
	I.tc0		= v2p_lmape_tc0;
	I.tc1		= v2p_lmape_tc1;
	I.tc2		= v2p_lmape_tc2;
	I.tc3		= v2p_lmape_tc3;
	I.c0	 	= v2p_lmape_c0;
	I.c1	 	= v2p_lmape_c1;
	I.fog		= v2p_lmape_fog;

	SV_Target	= _main (I);
}
