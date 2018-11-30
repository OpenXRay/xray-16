#include "common.h"
#include "iostructs\v_lod.h"

#define L_SCALE (2.0*1.55)
v2p _main ( v_lod I )
{
	v2p 		o;

	I.sun_af	= unpack_D3DCOLOR(I.sun_af);
	I.rgbh0		= unpack_D3DCOLOR(I.rgbh0);
	I.rgbh1		= unpack_D3DCOLOR(I.rgbh1);

	// lerp pos
	float	factor	= I.sun_af.w;
	float4	pos	= float4	(lerp(I.pos0,I.pos1,factor), 1.0);

	float	h	= lerp		(I.rgbh0.w,I.rgbh1.w,factor)*L_SCALE;

	o.hpos		= mul		(m_VP, 	pos);				// xform, input in world coords
	o.Pe		= mul		(m_V,	pos);

	// replicate TCs
	o.tc0		= I.tc0;						
	o.tc1		= I.tc1;						

	// calc normal & lighting
	o.af		= float4	(h, h, I.sun_af.z, factor);
	return	o;
}

