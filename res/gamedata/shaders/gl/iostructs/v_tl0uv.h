
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITION)		in float4	v_TL0uv_P		; // POSITION;
layout(location = COLOR)		in float4	v_TL0uv_Color	; // COLOR; 

layout(location = COLOR)	 	out float4	v2p_TL0uv_Color	; // COLOR;

v2p_TL0uv _main ( v_TL0uv I );

void main()
{
	v_TL0uv		I;
	I.P			= v_TL0uv_P;
	I.Color 	= v_TL0uv_Color;

	v2p_TL0uv O = _main (I);

	v2p_TL0uv_Color = O.Color;
	gl_Position = O.HPos;
}
