out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif // GBUFFER_OPTIMIZATION

struct	p_TL2uv_msaa
{
	float2 	Tex0	; // TEXCOORD0;
	float2	Tex1	; // TEXCOORD1;
	float4	Color	; // COLOR;
#ifdef GBUFFER_OPTIMIZATION
	float4 	HPos	; // SV_Position;	// Clip-space position 	(for rasterization)
#endif // GBUFFER_OPTIMIZATION
};

layout(location = TEXCOORD0)		in float2	p_TL0uv_Tex0	; // TEXCOORD0;
layout(location = TEXCOORD1)		in float2	p_TL0uv_Tex1	; // TEXCOORD1;
layout(location = COLOR)		in float4	p_TL0uv_Color	; // COLOR;

float4 _main ( p_TL2uv_msaa I );

void main()
{
	p_TL2uv_msaa	I;
	I.Tex0		= p_TL0uv_Tex0;
	I.Tex1		= p_TL0uv_Tex1;
	I.Color		= p_TL0uv_Color;
#ifdef GBUFFER_OPTIMIZATION
	I.HPos		= gl_FragCoord;
#endif // GBUFFER_OPTIMIZATION
	SV_Target	= _main (I);
}
