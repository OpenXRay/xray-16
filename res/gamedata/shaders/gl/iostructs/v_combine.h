
out gl_PerVertex { vec4 gl_Position; };

struct 	_in        	
{
	float4	P	; // POSITIONT;	// xy=pos, zw=tc0
	float2	tcJ	; // TEXCOORD0;	// jitter coords
};

struct 	v2p
{
#ifdef USE_VTF
	float4	tc0	; // TEXCOORD0;	// tc.xy, tc.w = tonemap scale
#else // USE_VTF
	float2	tc0	; // TEXCOORD0;	// tc.xy
#endif // USE_VTF
	float2	tcJ	; // TEXCOORD1;	// jitter coords
	float4	hpos	; // SV_Position;
};

layout(location = POSITIONT)	in float4	v_combine_P	; // POSITIONT;	// xy=pos, zw=tc0
layout(location = TEXCOORD0)	in float2	v_combine_tcJ	; // TEXCOORD0;	// jitter coords

#ifdef USE_VTF
layout(location = TEXCOORD0) 	out float4	v2p_combine_tc0	; // TEXCOORD0;	// tc.xy, tc.w = tonemap scale
#else // USE_VTF
layout(location = TEXCOORD0) 	out float2	v2p_combine_tc0	; // TEXCOORD0;	// tc.xy
#endif // USE_VTF
layout(location = TEXCOORD1) 	out float2	v2p_combine_tcJ	; // TEXCOORD1;	// jitter coords

v2p _main (_in v);

void main()
{
	_in		I;
	I.P		= v_combine_P;
	I.tcJ		= v_combine_tcJ;

	v2p O		= _main (I);

	v2p_combine_tc0	= O.tc0;
	v2p_combine_tcJ	= O.tcJ;
	gl_Position	= O.hpos;
}
