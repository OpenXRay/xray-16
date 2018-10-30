#include "common.h"
#include "shared\waterconfig.h"
#include "shared\watermove.h"
#include "iostructs\v_waterd.h"

uniform float4x4	m_texgen;

v2p _main (v_vert v)
{
	v.N		= unpack_D3DCOLOR(v.N);
	v.T		= unpack_D3DCOLOR(v.T);
	v.B		= unpack_D3DCOLOR(v.B);
	v.color		= unpack_D3DCOLOR(v.color);

	v2p 		o;

	float4 	P 	= v.P;
	float3 	N 	= unpack_normal		(v.N);
		P 	= watermove		(P);

	o.tbase		= unpack_tc_base	(v.uv,v.T.w,v.B.w);		// copy tc
	o.tdist0	= watermove_tc 		(o.tbase*W_DISTORT_BASE_TILE_0, P.xz, W_DISTORT_AMP_0);
	o.tdist1	= watermove_tc 		(o.tbase*W_DISTORT_BASE_TILE_1, P.xz, W_DISTORT_AMP_1);
	o.hpos 		= mul			(m_VP, P);			// xform, input in world coords

#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	o.tctexgen	= mul			(m_texgen, P);
	float3	Pe	= mul			(m_V, P);
	o.tctexgen.z	= Pe.z;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)

	return o;
}
