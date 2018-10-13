
out vec4 SV_Target;
#ifdef MSAA_OPTIMIZATION
in int gl_SampleID;
#endif

struct 	v2p
{
	float3 	lightToPos	; // TEXCOORD0;	// light center to plane vector
	float3 	vPos		; // TEXCOORD1;	// position in camera space
	float 	fDensity	; // TEXCOORD2;	// plane density along Z axis
//	float2	tNoise 		; // TEXCOORD3;	// projective noise
};

layout(location = TEXCOORD0)	in float3 	v2p_lightToPos	; // TEXCOORD0;		// light center to plane vector
layout(location = TEXCOORD1)	in float3 	v2p_vPos		; // TEXCOORD1;		// position in camera space
layout(location = TEXCOORD2)	in float 	v2p_fDensity	; // TEXCOORD2;		// plane density along Z axis
//layout(location = TEXCOORD3)	in float2	v2p_tNoise 		; // TEXCOORD3;		// projective noise

#ifdef MSAA_OPTIMIZATION
float4 _main ( v2p I, uint iSample );
#else
float4 _main ( v2p I );
#endif

void main()
{
	v2p	I;
	I.lightToPos = v2p_lightToPos;
	I.vPos		= v2p_vPos;
	I.fDensity	= v2p_fDensity;
//	I.tNoise	= v2p_tNoise;

#ifdef MSAA_OPTIMIZATION
	SV_Target	= _main ( I, gl_SampleID );
#else
	SV_Target	= _main ( I );
#endif
}
