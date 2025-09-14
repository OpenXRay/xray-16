
out vec4 SV_Target;

layout(location = TEXCOORD0)	in float4 	p_filter_Tex0	; // TEXCOORD0;
layout(location = TEXCOORD1)	in float4	p_filter_Tex1	; // TEXCOORD1;
layout(location = TEXCOORD2)	in float4 	p_filter_Tex2	; // TEXCOORD2;
layout(location = TEXCOORD3)	in float4	p_filter_Tex3	; // TEXCOORD3;
layout(location = TEXCOORD4)	in float4 	p_filter_Tex4	; // TEXCOORD4;
layout(location = TEXCOORD5)	in float4	p_filter_Tex5	; // TEXCOORD5;
layout(location = TEXCOORD6)	in float4 	p_filter_Tex6	; // TEXCOORD6;
layout(location = TEXCOORD7)	in float4	p_filter_Tex7	; // TEXCOORD7;

float4 _main ( p_filter I );

void main()
{
	p_filter	I;
	I.Tex0		= p_filter_Tex0;
	I.Tex1		= p_filter_Tex1;
	I.Tex2		= p_filter_Tex2;
	I.Tex3		= p_filter_Tex3;
	I.Tex4		= p_filter_Tex4;
	I.Tex5		= p_filter_Tex5;
	I.Tex6		= p_filter_Tex6;
	I.Tex7		= p_filter_Tex7;

	SV_Target	= _main (I);
}
