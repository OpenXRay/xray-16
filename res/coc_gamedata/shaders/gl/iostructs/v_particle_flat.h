
out gl_PerVertex { vec4 gl_Position; };

struct 		vv
{
	float4	P		; // POSITION;
	float2	tc		; // TEXCOORD0;
	float4	c		; // COLOR0;
};

struct 		v2p_particle
{
	float4 		color	; // COLOR0;
	v2p_flat	base;
};

layout(location = POSITION)		in float4	v_particle_P	; // POSITION;
layout(location = TEXCOORD0)	in float2	v_particle_tc	; // TEXCOORD0;
layout(location = COLOR)		in float4	v_particle_c	; // COLOR; 

layout(location = COLOR0) 		out float4	v2p_particle_color; // COLOR0;
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
layout(location = TEXCOORD0) 	out float4	v2p_flat_tcdh	; // TEXCOORD0;	// Texture coordinates,         w=sun_occlusion
#else
layout(location = TEXCOORD0) 	out float2	v2p_flat_tcdh	; // TEXCOORD0;	// Texture coordinates
#endif
layout(location = TEXCOORD1) 	out float4	v2p_flat_position; // TEXCOORD1;	// position + hemi
layout(location = TEXCOORD2) 	out float3	v2p_flat_N		; // TEXCOORD2;	// Eye-space normal        (for lighting)
#ifdef USE_TDETAIL
layout(location = TEXCOORD3) 	out float2	v2p_flat_tcdbump; // TEXCOORD3;	// d-bump
#endif
#ifdef USE_LM_HEMI
layout(location = TEXCOORD4) 	out float2	v2p_flat_lmh	; // TEXCOORD4;	// lm-hemi
#endif

v2p_particle _main ( vv I );

void main()
{
	vv		I;
	I.P		= v_particle_P;
	I.tc	= v_particle_tc;
	I.c 	= v_particle_c;

	v2p_particle O = _main (I);

	v2p_particle_color = O.color;
	v2p_flat_tcdh = O.base.tcdh;
	v2p_flat_position = O.base.position;
	v2p_flat_N	= O.base.N;
#ifdef USE_TDETAIL
	v2p_flat_tcdbump = O.base.tcdbump;
#endif
#ifdef USE_LM_HEMI
	v2p_flat_lmh = O.base.lmh;
#endif
	gl_Position = O.base.hpos;
}
