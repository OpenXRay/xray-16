
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITION)		in float4	v_dumb_P	; // POSITION;	// Clip-space position 	(for rasterization)

v2p_dumb _main ( v_dumb I );

void main()
{
	v_dumb		I;
	I.P			= v_dumb_P;

	v2p_dumb O 	= _main (I);

	gl_Position = O.HPos;
}
