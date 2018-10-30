#include "common.h"
#include "iostructs\v_lod.h"

#define L_SCALE (2.0f*1.55f)
v2p _main ( v_lod I )
{
	v2p 		o;

	//I.sun_af.xyz	= I.sun_af.zyx; // skyloader: is unpack_D3DCOLOR needed here?
	//I.rgbh0.xyz	= I.rgbh0.zyx;
	//I.rgbh1.xyz	= I.rgbh1.zyx;

	// lerp pos
	float	factor	= I.sun_af.w;
	float4	pos	= float4	(lerp(I.pos0,I.pos1,factor), 1.f);

	float	h	= lerp		(I.rgbh0.w,I.rgbh1.w,factor)*L_SCALE;

	o.hpos		= mul		(m_VP, 	pos);				// xform, input in world coords
	o.Pe		= mul		(m_V,	pos);

	// replicate TCs
	o.tc0		= I.tc0;						
	o.tc1		= I.tc1;						

	// calc normal & lighting
	o.af		= float4	(h,h,I.sun_af.z,factor);
	return	o;
}

