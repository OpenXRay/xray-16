
out gl_PerVertex { vec4 gl_Position; };

struct 	vi
{
	float4	p		; // POSITION;
	float4	dir		; // COLOR0;	// dir0,dir1(w<->z)
	float4	color	; // COLOR1;	// rgb. intensity
};

struct 	vf
{
	float4	color	; // COLOR0;	// rgb. intensity, for SM3 - tonemap-prescaled, HI-res
  	float2	tc0		; // TEXCOORD0;
  	float2	tc1		; // TEXCOORD1;
	float4 	hpos	; // SV_Position;
};

layout(location = POSITION)		in float4	v_clouds_p		; // POSITION;
layout(location = COLOR0)		in float4	v_clouds_dir	; // COLOR0;	// dir0,dir1(w<->z)
layout(location = COLOR1)		in float4	v_clouds_color	; // COLOR1;	// rgb. intensity

layout(location = COLOR0) 		out float4	v2p_clouds_color	; // COLOR0;	// rgb. intensity, for SM3 - tonemap-prescaled, HI-res
layout(location = TEXCOORD0) 	out float2	v2p_clouds_tc0		; // TEXCOORD0;
layout(location = TEXCOORD1) 	out float2	v2p_clouds_tc1		; // TEXCOORD1;

vf _main (vi v);

void main()
{
	vi		I;
	I.p		= v_clouds_p;
	I.dir	= v_clouds_dir;
	I.color = v_clouds_color;

	vf O 	= _main (I);

	v2p_clouds_color = O.color;
	v2p_clouds_tc0 = O.tc0;
	v2p_clouds_tc1 = O.tc1;
	gl_Position 	= O.hpos;
}
