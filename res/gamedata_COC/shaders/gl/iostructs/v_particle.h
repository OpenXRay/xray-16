
out gl_PerVertex { vec4 gl_Position; };

struct vv
{
	float4 P	; // POSITION;
	float2 tc	; // TEXCOORD0;
	float4 c	; // COLOR0;
};

struct v2p
{
	float2 tc	; // TEXCOORD0;
	float4 c	; // COLOR0;

//	Igor: for additional depth dest
#ifdef	USE_SOFT_PARTICLES
	float4 tctexgen	; // TEXCOORD1;
#endif	//	USE_SOFT_PARTICLES

	float4 hpos	; // SV_Position;
};

layout(location = POSITION)		in float4	v_particle_P	; // POSITION;
layout(location = TEXCOORD0)	in float2	v_particle_tc	; // TEXCOORD0;
layout(location = COLOR)		in float4	v_particle_c	; // COLOR; 

layout(location = TEXCOORD0) 	out float2	v2p_particle_tc	; // TEXCOORD0;
layout(location = COLOR0) 		out float4	v2p_particle_c	; // COLOR0;
#ifdef	USE_SOFT_PARTICLES
layout(location = TEXCOORD1) 	out float4	v2p_particle_tctexgen; // TEXCOORD1;
#endif	//	USE_SOFT_PARTICLES

v2p _main ( vv I );

void main()
{
	vv		I;
	I.P		= v_particle_P;
	I.tc	= v_particle_tc;
	I.c 	= v_particle_c;

	v2p O 	= _main (I);

	v2p_particle_tc = O.tc;
	v2p_particle_c = O.c;
#ifdef	USE_SOFT_PARTICLES
	v2p_particle_tctexgen = O.tctexgen;
#endif
	gl_Position = O.hpos;
}
