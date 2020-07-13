
out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif

struct v2p
{
	float2	tbase	; // TEXCOORD0;		// base
	float2	tnorm0	; // TEXCOORD1;		// nm0
	float2	tnorm1	; // TEXCOORD2;		// nm1
	float3	M1	; // TEXCOORD3;
	float3	M2	; // TEXCOORD4;
	float3	M3	; // TEXCOORD5;
	float3	v2point	; // TEXCOORD6;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	float4	tctexgen; // TEXCOORD7;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
#if SSR_QUALITY > 0
	float4	position_w;	// POSITION0;
#endif
	float4	c0	; // COLOR0;
	float	fog	; // FOG;
};

layout(location = TEXCOORD0) 		in float2	v2p_water_tbase		; // TEXCOORD0;
layout(location = TEXCOORD1) 		in float2	v2p_water_tnorm0	; // TEXCOORD1;
layout(location = TEXCOORD2) 		in float2	v2p_water_tnorm1	; // TEXCOORD2;
layout(location = TEXCOORD3) 		in float3	v2p_water_M1		; // TEXCOORD3;
layout(location = TEXCOORD4) 		in float3	v2p_water_M2		; // TEXCOORD4;
layout(location = TEXCOORD5) 		in float3	v2p_water_M3		; // TEXCOORD5;
layout(location = TEXCOORD6) 		in float3	v2p_water_v2point	; // TEXCOORD6;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
layout(location = TEXCOORD7) 		in float4	v2p_water_tctexgen	; // TEXCOORD7;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
#if SSR_QUALITY > 0
layout(location = POSITION0) 		in float4	v2p_water_pos	; // POSITION0;
#endif
layout(location = COLOR0) 		in float4	v2p_water_c0		; // COLOR0;
layout(location = FOG) 			in float	v2p_water_fog		; // FOG;

#ifdef GBUFFER_OPTIMIZATION
float4 _main( v2p I, float4 pos2d );
#else
float4 _main( v2p I );
#endif

void main()
{
	v2p		I;

	I.tbase		= v2p_water_tbase;
	I.tnorm0	= v2p_water_tnorm0;
	I.tnorm1	= v2p_water_tnorm1;
	I.M1		= v2p_water_M1;
	I.M2		= v2p_water_M2;
	I.M3		= v2p_water_M3;
	I.v2point	= v2p_water_v2point;
#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	I.tctexgen	= v2p_water_tctexgen;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
#if SSR_QUALITY > 0
    I.position_w = v2p_water_pos;
#endif
	I.c0		= v2p_water_c0;
	I.fog		= v2p_water_fog;
#ifdef GBUFFER_OPTIMIZATION
	SV_Target	= _main ( I, gl_FragCoord );
#else
	SV_Target	= _main ( I );
#endif
}
