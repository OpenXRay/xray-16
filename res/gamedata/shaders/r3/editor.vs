#include "common.h"

struct vf
{
	float4 C	: COLOR0;
	float4 P	: POSITION;
};

struct v2p
{
	float4 C	: COLOR0;
	float4 P	: SV_Position;
};

uniform float4 		tfactor;
// Packed COLOR0 is BGRA in the level vertex stream.
float4 unpack_D3DCOLOR(float4 color) { return color.bgra; }
v2p main (vf i)
{
	v2p 		o;

	o.P 		= mul			(m_WVP, i.P);			// xform, input in world coords
	o.C 		= tfactor*unpack_D3DCOLOR(i.C);

	return o;
}
