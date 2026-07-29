#include "common.h"

// effects\sun — same path as flare (additive sun disc / shafts)
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
