#include "common.h"

uniform float4 consts; // {scale, scale, scale, 1}
uniform float4 xform;

// New vertex structure for instanced details
struct v_detail_instanced
{
	float4 pos : POSITION;    // position.xyz, frac (normalized height) in w
	float2 tc  : TEXCOORD0;   // texture coordinates
};

// Instance data structure (must match C++ InstanceData)
// Phase 1, Milestone 1.2: Added object_id for unified geometry (not used yet)
struct InstanceData
{
	float3 hpb;      // Heading, pitch, bank rotation
	float scale;     // Scale factor
	float3 pos;      // Position
	float hemi;      // Hemisphere lighting
	uint object_id;  // Index into geometry offset table (for Milestone 1.3)
	uint3 padding;   // Pad to 16-byte alignment
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

	float4 pos;
	pos.x = dot(m0, float4(I.pos.xyz, 1.0));
	pos.y = dot(m1, float4(I.pos.xyz, 1.0));
	pos.z = dot(m2, float4(I.pos.xyz, 1.0));
	pos.w = 1.0f;

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
