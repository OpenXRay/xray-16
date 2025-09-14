
out vec4 SV_Target;

layout(location = TEXCOORD1)	in float2	p_shadow_tc0	; // TEXCOORD1;	// Diffuse map for aref

float4 _main ( p_shadow_direct_aref I );

void main()
{
	p_shadow_direct_aref I;
	I.tc0		= p_shadow_tc0;

	SV_Target	= _main (I);
}
