#ifndef        COMMON_H
#define        COMMON_H

#include "shared\common.h"

// TODO: OGL: Move to cbuffers
uniform half4                hemi_cube_pos_faces;
uniform half4                hemi_cube_neg_faces;
uniform half4                L_material;                            // 0,0,0,mid
uniform half4                Ldynamic_color;                      // dynamic light color (rgb1)        - spot/point
uniform half4                Ldynamic_pos;                       // dynamic light pos+1/range(w) - spot/point
uniform half4                Ldynamic_dir;                        // dynamic light direction         - sun

#include "common_defines.h"
#include "common_policies.h"
#include "common_iostructs.h"
#include "common_samplers.h"
//include "common_cbuffers.h"
#include "common_functions.h"

// #define USE_SUPER_SPECULAR

#ifdef        USE_R2_STATIC_SUN
#  define xmaterial float(1.0f/4.fs)
#else
#  define xmaterial float(L_material.w)
#endif

#endif
