
out vec4 SV_Target;
#ifdef GBUFFER_OPTIMIZATION
in vec4 gl_FragCoord;
#endif // GBUFFER_OPTIMIZATION

layout(location = TEXCOORD0)	in float2 	p_aa_AA_Tex0	; // TEXCOORD0;
layout(location = TEXCOORD1)	in float2	p_aa_AA_Tex1	; // TEXCOORD1;
layout(location = TEXCOORD2)	in float2 	p_aa_AA_Tex2	; // TEXCOORD2;
layout(location = TEXCOORD3)	in float2	p_aa_AA_Tex3	; // TEXCOORD3;
layout(location = TEXCOORD4)	in float2 	p_aa_AA_Tex4	; // TEXCOORD4;
layout(location = TEXCOORD5)	in float4	p_aa_AA_Tex5	; // TEXCOORD5;
layout(location = TEXCOORD6)	in float4 	p_aa_AA_Tex6	; // TEXCOORD6;

#ifdef GBUFFER_OPTIMIZATION
float4 _main ( v_aa_AA I, float4 pos2d );
#else // GBUFFER_OPTIMIZATION
float4 _main ( v_aa_AA I );
#endif // GBUFFER_OPTIMIZATION

void main()
{
	v_aa_AA	I;
#ifdef GBUFFER_OPTIMIZATION
	I.P			= gl_FragCoord;
#endif // GBUFFER_OPTIMIZATION
	I.Tex0		= p_aa_AA_Tex0;
	I.Tex1		= p_aa_AA_Tex1;
	I.Tex2		= p_aa_AA_Tex2;
	I.Tex3		= p_aa_AA_Tex3;
	I.Tex4		= p_aa_AA_Tex4;
	I.Tex5		= p_aa_AA_Tex5;
	I.Tex6		= p_aa_AA_Tex6;

#ifdef GBUFFER_OPTIMIZATION
	SV_Target	= _main ( I, gl_FragCoord );
#else // GBUFFER_OPTIMIZATION
	SV_Target	= _main ( I );
#endif // GBUFFER_OPTIMIZATION
}
