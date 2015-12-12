
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITIONT)	in float4	v_TL2uv_P		; // POSITIONT;
layout(location = TEXCOORD0)	in float2	v_TL2uv_Tex0	; // TEXCOORD0;
layout(location = TEXCOORD1)	in float2	v_TL2uv_Tex1	; // TEXCOORD1;
layout(location = COLOR)		in float4	v_TL2uv_Color	; // COLOR; 

layout(location = TEXCOORD0) 	out float2 	v2p_TL2uv_Tex0	; // TEXCOORD0;
layout(location = TEXCOORD1) 	out float2	v2p_TL2uv_Tex1	; // TEXCOORD1;
layout(location = COLOR)	 	out float4	v2p_TL2uv_Color	; // COLOR;

v2p_TL2uv _main ( v_TL2uv I );

void main()
{
	v_TL2uv		I;
	I.P			= v_TL2uv_P;
	I.Tex0		= v_TL2uv_Tex0;
	I.Tex1		= v_TL2uv_Tex1;
	I.Color 	= v_TL2uv_Color;

	v2p_TL2uv O 	= _main (I);

	v2p_TL2uv_Tex0 = O.Tex0;
	v2p_TL2uv_Tex1 = O.Tex1;
	v2p_TL2uv_Color = O.Color;
	gl_Position = O.HPos;
}
