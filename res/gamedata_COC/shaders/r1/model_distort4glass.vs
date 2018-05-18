#include "common.h"
#include "skin.h"

struct vf
{
  float4 hpos  : POSITION;
  float2 tc0  : TEXCOORD0;    // base
  float4 c0  : COLOR0;      // color
};

vf   _main (v_model v)
{
  vf     o;

  o.hpos       = mul      (m_WVP, v.pos);      // xform, input in world coords
  o.tc0      = v.tc.xy;              // copy tc

  // calculate fade
  float3  dir_v   = normalize    (mul(m_WV,v.pos));
  float3  norm_v  = normalize    (mul(m_WV,v.norm));
  float   fade    = 0.9*abs      (dot(dir_v,norm_v));
  o.c0        = fade;

  return o;
}

/////////////////////////////////////////////////////////////////////////
#define SKIN_VF vf
#include "skin_main.h"