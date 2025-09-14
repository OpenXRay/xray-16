
out gl_PerVertex { vec4 gl_Position; };

struct v_lod
{
	float3 pos0	; 	// POSITION0;
	float3 pos1	; 	// POSITION1;
	float3 n0	; 	// NORMAL0;
	float3 n1	; 	// NORMAL1;
	float2 tc0	; 	// TEXCOORD0;
	float2 tc1	; 	// TEXCOORD1;
	float4 rgbh0	; 	// TEXCOORD2;		// rgb.h
	float4 rgbh1	; 	// TEXCOORD3;		// rgb.h
	float4 sun_af	; 	// COLOR0;		// x=sun_0, y=sun_1, z=alpha, w=factor
};
struct v2p
{
	float4 	hpos	; 	// SV_Position;
	float3	Pe	; 	// TEXCOORD0;
 	float2 	tc0	; 	// TEXCOORD1;		// base0
 	float2 	tc1	; 	// TEXCOORD2;		// base1
	float4 	af	; 	// COLOR1;		// alpha&factor //skyloader: COLOR1? maybe COLOR0?
};

layout(location = POSITION0)		in float3	v_lod_pos0		; // POSITION0;		
layout(location = POSITION1)		in float3	v_lod_pos1		; // POSITION1;		
layout(location = NORMAL0)		in float3	v_lod_n0		; // NORMAL0;
layout(location = NORMAL1)		in float3	v_lod_n1		; // NORMAL1;
layout(location = TEXCOORD0)		in float2	v_lod_tc0		; // TEXCOORD0;
layout(location = TEXCOORD1)		in float2	v_lod_tc1		; // TEXCOORD1;
layout(location = TEXCOORD2)		in float4	v_lod_rgbh0		; // TEXCOORD2;		// rgb.h
layout(location = TEXCOORD3)		in float4	v_lod_rgbh1		; // TEXCOORD3;		// rgb.h
layout(location = COLOR0)		in float4	v_lod_sun_af		; // COLOR0;		// x=sun_0, y=sun_1, z=alpha, w=factor


layout(location = TEXCOORD0) 		out float3	v2p_lod_Pe		; // TEXCOORD0;		// base
layout(location = TEXCOORD1) 		out float2	v2p_lod_tc0		; // TEXCOORD1;		// lmap
layout(location = TEXCOORD2) 		out float2	v2p_lod_tc1		; // TEXCOORD2;		// hemi
layout(location = COLOR1) 		out float4	v2p_lod_af		; // COLOR1;

v2p _main ( v_lod I );

void main()
{
	v_lod		I;
	I.pos0		= v_lod_pos0;
	I.pos1		= v_lod_pos1;
	I.n0		= v_lod_n0;
	I.n1		= v_lod_n1;
	I.tc0		= v_lod_tc0;
	I.tc1		= v_lod_tc1;
	I.rgbh0		= v_lod_rgbh0;
	I.rgbh1		= v_lod_rgbh1;
	I.sun_af	= v_lod_sun_af;

	v2p O 		= _main (I);

	v2p_lod_Pe	= O.Pe;
	v2p_lod_tc0	= O.tc0;
	v2p_lod_tc1	= O.tc1;
	v2p_lod_af	= O.af;
	gl_Position	= O.hpos;
}
