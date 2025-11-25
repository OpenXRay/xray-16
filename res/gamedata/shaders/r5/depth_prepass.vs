// depth_prepass.vs - Minimal vertex shader for depth prepass
// Only transforms vertices to clip space, no other calculations needed

#include "common.h"

// Use the standard static vertex input
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
#define v_in v_static_color
#else
#define v_in v_static
#endif

// Minimal output - only position and texcoord (for alpha testing)
struct v2p_depth {
    float4 hpos : SV_POSITION;  // Homogeneous clip-space position
    float2 tc0  : TEXCOORD0;    // Base texture coordinates (for alpha test)
};

v2p_depth main(v_in I)
{
    v2p_depth O;

    // Transform vertex position to clip space
    float4 w_pos = I.P;
    O.hpos = mul(m_WVP, w_pos);

    // Unpack texture coordinates (needed for alpha testing in pixel shader)
    O.tc0 = unpack_tc_base(I.tc, I.T.w, I.B.w);

    return O;
}

FXVS;
