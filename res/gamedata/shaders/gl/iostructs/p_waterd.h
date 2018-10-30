
out vec4 SV_Target;

struct v2p
{
	float2  tbase	;	// TEXCOORD0;
	float2  tdist0	;	// TEXCOORD1;
	float2  tdist1	;	// TEXCOORD2;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	float4  tctexgen;	// TEXCOORD3;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
};

layout(location = TEXCOORD0) 		in float2	v2p_vert_tbase		; // TEXCOORD0;
layout(location = TEXCOORD1) 		in float2	v2p_vert_tdist0		; // TEXCOORD1;
layout(location = TEXCOORD2) 		in float2	v2p_vert_tdist1		; // TEXCOORD2;		
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
layout(location = TEXCOORD3) 		in float4	v2p_vert_tctexgen	; // TEXCOORD3;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)

float4 _main ( v2p I );

void main()
{
	v2p		I;
	I.tbase		= v2p_vert_tbase;
	I.tdist0	= v2p_vert_tdist0;
	I.tdist1	= v2p_vert_tdist1;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	I.tctexgen	= v2p_vert_tctexgen;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	SV_Target	= _main (I);
}
