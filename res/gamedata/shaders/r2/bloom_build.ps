#include "common.h"

struct         v2p
{
  float2 tc0:                TEXCOORD0;        // Texture coordinates         (for sampling maps)
  float2 tc1:                TEXCOORD1;        // Texture coordinates         (for sampling maps)
  float2 tc2:                TEXCOORD2;        // Texture coordinates         (for sampling maps)
  float2 tc3:                TEXCOORD3;        // Texture coordinates         (for sampling maps)
};

uniform float4                b_params;

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
float4         main                        ( v2p I )        : COLOR
{
        // hi-rgb.base-lum
        float3       	s0              =        tex2D           	(s_image,        I.tc0);
        float3         	s1              =        tex2D       		(s_image,        I.tc1);
        float3         	s2              =        tex2D       		(s_image,        I.tc2);
        float3         	s3             	=        tex2D    			(s_image,        I.tc3);


		float3			avg				= (s0+s1+s2+s3)/2;
		float 			hi				= dot(avg,1.h)-b_params.x	;	// assume def_hdr equal to 3.0

        return        	float4(avg,hi);
}
