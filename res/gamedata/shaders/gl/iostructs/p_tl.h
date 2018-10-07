
out vec4 SV_Target;

layout(location = TEXCOORD0)	in float2	p_TL_Tex0	; // TEXCOORD0;
layout(location = COLOR)		in float4	p_TL_Color	; // COLOR; 

float4 _main ( p_TL I );

void main()
{
	p_TL		I;
	I.Tex0		= p_TL_Tex0;
	I.Color 	= p_TL_Color;

	SV_Target	= _main (I);
}
