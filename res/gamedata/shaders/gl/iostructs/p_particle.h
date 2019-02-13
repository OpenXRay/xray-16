
out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif // GBUFFER_OPTIMIZATION

struct v2p
{
	float2 tc0	; // TEXCOORD0;
	float4 c	; // COLOR0;

//	Igor: for additional depth dest
#ifdef USE_SOFT_PARTICLES
	float4 tctexgen	; // TEXCOORD1;
#endif // USE_SOFT_PARTICLES
#ifdef GBUFFER_OPTIMIZATION
	float4 hpos	; // SV_Position;
#endif // USE_SOFT_PARTICLES
};

layout(location = TEXCOORD0) 	in float2	p_particle_tc	; // TEXCOORD0;
layout(location = COLOR0) 		in float4	p_particle_c	; // COLOR0;
#ifdef	USE_SOFT_PARTICLES
layout(location = TEXCOORD1) 	in float4	p_particle_tctexgen; // TEXCOORD1;
#endif	//	USE_SOFT_PARTICLES

float4 _main ( v2p I );

void main()
{
	v2p			I;
	I.tc0		= p_particle_tc;
	I.c 		= p_particle_c;
#ifdef USE_SOFT_PARTICLES
	I.tctexgen 	= p_particle_tctexgen;
#endif // USE_SOFT_PARTICLES
#ifdef GBUFFER_OPTIMIZATION
	I.hpos		= gl_FragCoord;
#endif // GBUFFER_OPTIMIZATION
	SV_Target	= _main (I);
}
