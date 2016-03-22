
out gl_PerVertex { vec4 gl_Position; };

struct 	a2v
{
// 	float4 tc0; //		TEXCOORD0;	// Texture coordinates
	float4 P; //	 	POSITION;	// Object-space position
};

layout(location = POSITION)		in float4	v_shadow_P		; // POSITION;	// Object-space position

v2p_shadow_direct _main ( a2v I );

void main()
{
	a2v		I;
	I.P		= v_shadow_P;

	v2p_shadow_direct O = _main (I);

	gl_Position = O.hpos;
}
