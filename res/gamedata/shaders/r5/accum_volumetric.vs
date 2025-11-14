#include "common.h"

cbuffer VolumetricLights
{
	float3	vMinBounds;
	float3	vMaxBounds;
	float4	FrustumClipPlane[6];
}

struct v2p
{
	float3 	lightToPos	: TEXCOORD0;
	float3 	vPos		: TEXCOORD1;
	float 	fDensity	: TEXCOORD2;
	float3	clip0		: SV_ClipDistance0;
	float3	clip1		: SV_ClipDistance1;
	float4 	hpos		: SV_Position;
};

v2p main ( float3 P : POSITION )
{
	v2p 		o;
	float4	vPos;
	vPos.xyz 	= lerp( vMinBounds, vMaxBounds, P);
	vPos.w 		= 1;
	o.hpos 		= mul			(m_P, vPos);

	o.lightToPos = vPos.xyz - Ldynamic_pos.xyz;
	o.vPos = vPos.xyz;

	o.fDensity = 1.0h/40.0h;

	for (int i=0; i<3; ++i)
	{
		o.clip0[i] = dot( o.hpos, FrustumClipPlane[i]);
		o.clip1[i] = dot( o.hpos, FrustumClipPlane[i+3]);
	}

	return o;
}