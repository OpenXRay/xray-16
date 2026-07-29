#include "common.h"

// effects\flare / effects\sun — world-space billboard (matches gl effects_sun.vs)
v2p_TL main(v_TL I)
{
	v2p_TL O;
	O.HPos = mul(m_VP, I.P);
	O.HPos.z = O.HPos.w;
	O.Tex0 = I.Tex0;
	O.Color = I.Color.bgra;
	O.TexIdx = I.TexIdx;
	return O;
}
