
out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif

struct v2p
{
	float2  tbase	; // TEXCOORD0;
	float2  tdist0	; // TEXCOORD1;
	float2  tdist1	; // TEXCOORD2;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	float4  tctexgen; // TEXCOORD3;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
#ifdef GBUFFER_OPTIMIZATION
	float4  hpos	;	// SV_Position;
#endif
};

layout(location = TEXCOORD0) 		in float2	v2p_waterd_tbase		; // TEXCOORD0;
layout(location = TEXCOORD1) 		in float2	v2p_waterd_tdist0		; // TEXCOORD1;
layout(location = TEXCOORD2) 		in float2	v2p_waterd_tdist1		; // TEXCOORD2;		
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
layout(location = TEXCOORD3) 		in float4	v2p_waterd_tctexgen		; // TEXCOORD3;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)

float4 _main ( v2p I );

void main()
{
	v2p		I;
	I.tbase		= v2p_waterd_tbase;
	I.tdist0	= v2p_waterd_tdist0;
	I.tdist1	= v2p_waterd_tdist1;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	I.tctexgen	= v2p_waterd_tctexgen;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
#ifdef GBUFFER_OPTIMIZATION
	I.hpos	= gl_FragCoord;
#endif
	SV_Target	= _main (I);
}
