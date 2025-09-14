
out vec4 SV_Target;

layout(location = COLOR)		in float4	p_TL0uv_Color	; // COLOR;

float4 _main ( p_TL0uv I );

void main()
{
	p_TL0uv		I;
	I.Color 	= p_TL0uv_Color;

	SV_Target = _main (I);
}
