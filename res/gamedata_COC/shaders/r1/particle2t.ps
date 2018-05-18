#include "common.h"

struct   v2p
{
   float2   tc0  :   TEXCOORD0;  // base
   float2   tc1  :   TEXCOORD1;  // another
  half4  c  :  COLOR0;    // diffuse
};

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
uniform sampler2D   s_another;
half4   main_ps_1_1  ( v2p I )  : COLOR
{
 half3 base = I.c*tex2D(s_base,I.tc0);
 half4 grad = tex2D(s_another,I.tc1);
 half3 mult = base*grad*2;
  return  half4( lerp(base,mult,grad.w), 1);
}
