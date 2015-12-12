
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITIONT)	in float4	v_postpr_P		; // POSITIONT;
layout(location = TEXCOORD0)	in float2 	v_postpr_Tex0	; // TEXCOORD0;	// base1 (duality)	
layout(location = TEXCOORD1)	in float2	v_postpr_Tex1	; // TEXCOORD1;	// base2 (duality)
layout(location = TEXCOORD2)	in float2	v_postpr_Tex2	; // TEXCOORD2;	// base  (noise)
layout(location = COLOR0)		in float4	v_postpr_Color	; // COLOR0;		// multiplier, color.w = noise_amount
layout(location = COLOR1)		in float4	v_postpr_Gray	; // COLOR1;		// (.3,.3,.3.,amount)

layout(location = TEXCOORD0) 	out float2 	v2p_postpr_Tex0	; // TEXCOORD0;	// base1 (duality)	
layout(location = TEXCOORD1) 	out float2	v2p_postpr_Tex1	; // TEXCOORD1;	// base2 (duality)
layout(location = TEXCOORD2) 	out float2	v2p_postpr_Tex2	; // TEXCOORD2;	// base  (noise)
layout(location = COLOR0)		out float4	v2p_postpr_Color; // COLOR0;		// multiplier, color.w = noise_amount
layout(location = COLOR1)		out float4	v2p_postpr_Gray	; // COLOR1;		// (.3,.3,.3.,amount)

v2p_postpr _main ( v_postpr I );

void main()
{
	v_postpr	I;
	I.P			= v_postpr_P;
	I.Tex0		= v_postpr_Tex0;
	I.Tex1 		= v_postpr_Tex1;
	I.Tex2 		= v_postpr_Tex2;
	I.Color		= v_postpr_Color;
	I.Gray 		= v_postpr_Gray;

	v2p_postpr O = _main (I);

	v2p_postpr_Tex0 = O.Tex0;
	v2p_postpr_Tex1 = O.Tex1;
	v2p_postpr_Tex2 = O.Tex2;
	v2p_postpr_Color = O.Color;
	v2p_postpr_Gray = O.Gray;
	gl_Position = O.HPos;
}
