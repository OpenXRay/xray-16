#include "common.h"

uniform float4 consts; // {1/quant,1/quant,diffusescale,ambient}
uniform float4 wave;   // cx,cy,cz,tm
uniform float4 dir2D;

// New vertex structure for instanced details
struct v_detail_instanced
{
	float4 pos : POSITION;    // position.xyz, frac (normalized height) in w
	float2 tc  : TEXCOORD0;   // texture coordinates
};

// Instance data structure (must match C++ InstanceData)
struct InstanceData
{
	float3 hpb;      // Heading, pitch, bank rotation
	float scale;     // Scale factor
	float3 pos;      // Position
	float hemi;      // Hemisphere lighting
	uint vis_id;     // Visibility/animation type (0=still, 1=wave1, 2=wave2)
	uint padding;    // Alignment padding
};

// Structured buffer bound to slot 0
StructuredBuffer<InstanceData> detail_buffer : register(t0);

float3x3 setMatrix(float3 hpb)
{
	float _ch, _cp, _cb, _sh, _sp, _sb, _cc, _cs, _sc, _ss;

	sincos(hpb.x, _sh, _ch);
	sincos(hpb.y, _sp, _cp);
	sincos(hpb.z, _sb, _cb);

	_cc = _cb * _ch;
	_cs = _cb * _sh;
	_sc = _sb * _ch;
	_ss = _sb * _sh;

	return float3x3(
		_cp * _ch, _sp * _sc - _cs, _sp * _cc + _ss,
		_cp * _sh, _sp * _ss + _cc, _sp * _cs - _sc,
		-_sp, _cp * _sb, _cp * _cb);
}

v2p_flat main(v_detail_instanced I, uint instance_id : SV_InstanceID)
{
	v2p_flat O;

	// Read instance data from structured buffer
	InstanceData det = detail_buffer[instance_id];

	float3x3 mmhpb = setMatrix(det.hpb);

	float hemi = abs(det.hemi);
	float sun = sign(det.hemi) * 0.25f + 0.25f;

	float4 m0 = float4(mmhpb[0] * det.scale, det.pos.x);
	float4 m1 = float4(mmhpb[1] * det.scale, det.pos.y);
	float4 m2 = float4(mmhpb[2] * det.scale, det.pos.z);

	float4 pos, pos_old;
	pos.x = dot(m0, float4(I.pos.xyz, 1.0));
	pos.y = dot(m1, float4(I.pos.xyz, 1.0));
	pos.z = dot(m2, float4(I.pos.xyz, 1.0));
	pos.w = 1.0f;
	pos_old = pos;

#ifdef USE_TREEWAVE
	float H = I.pos.y * length(m1.xyz);

	float dp = calc_cyclic(dot(pos, wave));
	float inten = H * dp;

	pos.xz += calc_xz_wave(dir2D.xz * inten, I.pos.w);

	#ifndef DETAIL_SHADOW_PASS
		float dp_old = calc_cyclic(dot(pos_old, wave_old));
		float inten_old = H * dp_old;

		pos_old.xz += calc_xz_wave(dir2D_old.xz * inten_old, I.pos.w);
	#endif
#endif

	float3 Pe = mul(m_WV, pos);

	float3 N;
	N.x = pos.x - m0.w;
	N.y = pos.y - m1.w + 0.75f;
	N.z = pos.z - m2.w;

	O.tcdh = float4(I.tc.xy, hemi, sun);
	O.position = float4(Pe, 1.0f);

	N.xyz = mul((float3x3)m_WV, N.xyz);
	O.N = N.xyz;

	O.hpos = mul(m_WVP, pos);
	return O;
}
FXVS;
