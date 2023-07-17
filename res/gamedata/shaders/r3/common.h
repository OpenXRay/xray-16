#ifndef        COMMON_H
#define        COMMON_H

#include "shared\common.h"

#include "common_defines.h"
#include "common_policies.h"
#include "common_iostructs.h"
#include "common_samplers.h"
#include "common_cbuffers.h"
#include "common_functions.h"

// #define USE_SUPER_SPECULAR


#ifdef        USE_R2_STATIC_SUN
#  define xmaterial float(1.0h/4.h)
#else
#  define xmaterial float(L_material.w)
#endif

/*
//////////////////////////////////////////////////////////////////////////////////////////
// *** options

// #define DBG_TEST_NMAP
// #define DBG_TEST_NMAP_SPEC
// #define DBG_TEST_SPEC
// #define DBG_TEST_LIGHT
// #define DBG_TEST_LIGHT_SPEC

// #define USE_GAMMA_22
// #define USE_SJITTER
// #define USE_SUNFILTER
// #define USE_FETCH4
// #define USE_MBLUR                	//- HW-options defined
// #define USE_HWSMAP                	//- HW-options defined

// #define USE_HWSMAP_PCF				//- nVidia GF3+, R600+

// #define USE_BRANCHING        		//- HW-options defined
// #define USE_VTF                		//- HW-options defined, VertexTextureFetch
// #define FP16_FILTER                	//- HW-options defined
// #define FP16_BLEND                	//- HW-options defined
//
// #define USE_PARALLAX                	//- shader defined
// #define USE_TDETAIL                	//- shader defined
// #define USE_LM_HEMI                	//- shader defined
// #define USE_DISTORT                	//- shader defined
// #define USE_SUNMASK                		//- shader defined
// #define DBG_TMAPPING
//////////////////////////////////////////////////////////////////////////////////////////

uniform float4                J_direct        [6];
uniform float4                J_spot                [6];

float2         calc_detail                (float3 w_pos)      {
        float                 dtl        = distance                (w_pos,eye_position)*dt_params.w;
                              dtl        = min              (dtl*dtl, 1);
        float                  dt_mul     = 1  - dtl;        // dt*  [1 ..  0 ]
        float                  dt_add     = .5 * dtl;        // dt+  [0 .. 0.5]
        return                float2      (dt_mul,dt_add);
}


//////////////////////////////////////////////////////////////////////////////////////////

#ifdef  USE_HWSMAP
#else
struct         v_shadow_direct_aref
{
        float4      hpos:        POSITION;       // Clip-space position         (for rasterization)
        float       depth:         TEXCOORD0;     // Depth
        float2      tc0:        TEXCOORD1;       // Diffuse map for aref
};
struct         v_shadow_direct
{
        float4      hpos:        POSITION;       // Clip-space position         (for rasterization)
        float       depth:         TEXCOORD0;     // Depth
};


#endif

//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
struct  				p_screen                {
        float4          hpos               		: POSITION;
        float2          tc0                		: TEXCOORD0;        // Texture coordinates         (for sampling maps)
};


//////////////////////////////////////////////////////////////////////////////////////////

float3	v_hemi_wrap     (float3 n, float w)                	{        return L_hemi_color*(w + (1-w)*n.y);                   }
float3	v_sun_wrap      (float3 n, float w)                	{        return L_sun_color*(w+(1-w)*dot(n,-L_sun_dir_w));      }
*/
#define FXPS technique _render{pass _code{PixelShader=compile ps_3_0 main();}}
#define FXVS technique _render{pass _code{VertexShader=compile vs_3_0 main();}}

#endif
