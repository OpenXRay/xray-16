#include "common.h"

v2p_TL main(v_TL I)
{
    v2p_TL O;

    O.HPos = mul(m_VP, I.P);
    O.HPos.z = O.HPos.w * 0.0001;
    O.Tex0 = I.Tex0;
    O.Color = I.Color.bgra;

    return O;
}
