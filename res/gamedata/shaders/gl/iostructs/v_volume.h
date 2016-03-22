
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITION)		in float4	v_volume_P;

layout(location = TEXCOORD0)	out float4 	v2p_volume_tc	; // TEXCOORD0;
#ifdef 	USE_SJITTER
layout(location = TEXCOORD1)	out float4 	v2p_volume_tcJ	; // TEXCOORD1;
#endif

v2p_volume _main ( float4 P );

void main()
{
	v2p_volume O	= _main ( v_volume_P );
	v2p_volume_tc	= O.tc;
#ifdef 	USE_SJITTER
	v2p_volume_tcJ	= O.tcJ;
#endif
	gl_Position		= O.hpos;
}
