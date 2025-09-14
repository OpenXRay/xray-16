#include "common.h"
#include "iostructs\v_editor.h"

uniform float4 		tfactor;
v2p _main (vf i)
{
	v2p 		o;

	o.P 		= mul			(m_WVP, i.P);			// xform, input in world coords
	o.C 		= tfactor*unpack_D3DCOLOR(i.C);

	return o;
}
