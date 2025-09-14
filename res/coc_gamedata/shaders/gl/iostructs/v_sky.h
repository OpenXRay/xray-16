
out gl_PerVertex { vec4 gl_Position; };

struct vi
{
	float4	p		; // POSITION;
	float4	c		; // COLOR0;
	float3	tc0		; // TEXCOORD0;
	float3	tc1		; // TEXCOORD1;
};

struct v2p
{
	float4	c		; // COLOR0;
	float3	tc0		; // TEXCOORD0;
	float3	tc1		; // TEXCOORD1;
	float4	hpos	; // SV_Position;
};

layout(location = POSITION)		in float4	v_sky_p		; // POSITION;
layout(location = COLOR0)		in float4	v_sky_c		; // COLOR0;
layout(location = TEXCOORD0)	in float3	v_sky_tc0	; // TEXCOORD0;
layout(location = TEXCOORD1)	in float3	v_sky_tc1	; // TEXCOORD1;

layout(location = COLOR0) 		out float4	v2p_sky_c	; // COLOR0;
layout(location = TEXCOORD0) 	out float3	v2p_sky_tc0	; // TEXCOORD0;
layout(location = TEXCOORD1) 	out float3	v2p_sky_tc1	; // TEXCOORD1;

v2p _main (vi v);

void main()
{
	vi		I;
	I.p		= v_sky_p;
	I.c		= v_sky_c;
	I.tc0 	= v_sky_tc0;
	I.tc1 	= v_sky_tc1;

	v2p O 	= _main (I);

	v2p_sky_c	= O.c;
	v2p_sky_tc0 = O.tc0;
	v2p_sky_tc1 = O.tc1;
	gl_Position = O.hpos;
}
