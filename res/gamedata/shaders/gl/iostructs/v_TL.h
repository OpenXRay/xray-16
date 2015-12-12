
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITION)		in float4	v_TL_P		; // POSITION;
layout(location = TEXCOORD0)	in float2	v_TL_Tex0	; // TEXCOORD0;
layout(location = COLOR)		in float4	v_TL_Color	; // COLOR; 

layout(location = TEXCOORD0) 	out float2 	v2p_TL_Tex0	; // TEXCOORD0;
layout(location = COLOR) 		out float4	v2p_TL_Color; // COLOR;

v2p_TL _main ( v_TL I );

void main()
{
	v_TL		I;
	I.P			= v_TL_P;
	I.Tex0		= v_TL_Tex0;
	I.Color 	= v_TL_Color;

	v2p_TL O 	= _main (I);

	v2p_TL_Tex0	= O.Tex0;
	v2p_TL_Color = O.Color;
	gl_Position = O.HPos;
}
