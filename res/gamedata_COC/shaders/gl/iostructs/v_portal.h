
out gl_PerVertex { vec4 gl_Position; };

struct	v_vert
{
	float4 	pos	; // POSITION;	// (float,float,float,1)
	float4	color	; // COLOR0;	// (r,g,b,dir-occlusion)
};
struct 	v2p
{
	float4 c	; // COLOR0;
	float  fog	; // FOG;
	float4 hpos	; // SV_Position;
};

layout(location = POSITION)		in float4	v_portal_pos	; // POSITION;	// (float,float,float,1)
layout(location = COLOR0)		in float4	v_portal_color	; // COLOR0;	// (r,g,b,dir-occlusion)

layout(location = COLOR0) 		out float4	v2p_portal_c	; // COLOR0;
layout(location = FOG) 			out float	v2p_portal_fog	; // FOG;

v2p _main ( v_vert I );

void main()
{
	v_vert		I;
	I.pos		= v_portal_pos;
	I.color		= v_portal_color;

	v2p O 		= _main (I);

	v2p_portal_c = O.c;
	v2p_portal_fog = O.fog;
	gl_Position = O.hpos;
}
